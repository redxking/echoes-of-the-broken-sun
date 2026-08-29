#include "EchoesPlayerController.h"

#include "EchoesCommandMarkerView.h"
#include "EchoesEntityView.h"
#include "EchoesGameUserSettings.h"
#include "EchoesOfTheBrokenSun.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesTechnologyPanelLayout.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

namespace
{
constexpr float DragSelectionThresholdPixels = 8.0f;
constexpr float FormationSpacingWorldUnits = 150.0f;
constexpr int32 ControlGroupCount = 10;

[[nodiscard]] FString FactionDisplayName(echoes::sim::Faction Faction)
{
    return Faction == echoes::sim::Faction::KharuunAssemblies
               ? TEXT("KHARUUN ASSEMBLIES")
               : TEXT("MERIDIAN COMPACT");
}
}

AEchoesPlayerController::AEchoesPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = false;
    bEnableMouseOverEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AEchoesPlayerController::BeginPlay()
{
    Super::BeginPlay();

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    bShowMouseCursor = true;
    if (!bRuntimeStateKnown)
    {
        const UEchoesSimulationSubsystem* Bridge =
            GetWorld() != nullptr
                ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
                : nullptr;
        if (Bridge != nullptr && Bridge->IsScenarioReady())
        {
            NotifyRuntimeReady();
        }
        else
        {
            SetStatusMessage(
                TEXT("Initializing runtime technical prototype..."),
                15.0f);
        }
    }
}

void AEchoesPlayerController::NotifyRuntimeReady()
{
    bRuntimeStateKnown = true;
    SetStatusMessage(
        FString::Printf(
            TEXT("Runtime prototype ready. Select owned %s units, then right-click a destination or target."),
            *GetLocalFactionLabel()),
        7.0f);
}

FString AEchoesPlayerController::GetLocalFactionLabel() const
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return FactionDisplayName(
        Bridge != nullptr
            ? Bridge->GetLocalFaction()
            : echoes::sim::Faction::MeridianCompact);
}

FString AEchoesPlayerController::GetOpponentFactionLabel() const
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    return FactionDisplayName(
        Bridge != nullptr
            ? Bridge->GetOpponentFaction()
            : echoes::sim::Faction::KharuunAssemblies);
}

void AEchoesPlayerController::PresentTitleScreen()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[TITLE_SIM_NOT_READY] The operation is unavailable."));
        return;
    }
    ClearSelection();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bTitleScreenVisible = true;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = false;
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    bCampaignResult = false;
    bCampaignSuccess = false;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    Bridge->SetScenarioPaused(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("ECHOES OF THE BROKEN SUN — F9 changes operation; %sEnter opens the brief."),
            Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
                ? TEXT("Tab changes faction; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignPrologue
                ? TEXT("Mara Vey deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT("Oruun deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignCityReserve
                ? TEXT("Mara Vey deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT("Oruun deployed; ")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("Meridian treaty proxies deployed; ")
                : TEXT("Talar and two civilian proxies deployed; ")),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_TITLE_READY] operation=%s operationChoice=true keyboardStart=true factionChoice=%s"),
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue
            ? TEXT("WhatTheLedgerKeeps")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignSevenAccounts
            ? TEXT("SevenAccountsOfRain")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignCityReserve
            ? TEXT("ACityOnReserve")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignUnburiedRoad
            ? TEXT("TheUnburiedRoad")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignTermsOfContinuance
            ? TEXT("TermsOfContinuance")
        : Bridge->GetOperationMode() ==
                EEchoesOperationMode::CampaignNamesWithoutBirths
            ? TEXT("NamesWithoutBirths")
            : TEXT("GlassScar"),
        Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish
            ? TEXT("true")
            : TEXT("false"));
}

void AEchoesPlayerController::ConfirmTitleScreen()
{
    if (!bTitleScreenVisible)
    {
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    bTitleScreenVisible = false;
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_TITLE_CONFIRMED] next=OperationsBrief"));
    PresentMissionBriefing();
}

void AEchoesPlayerController::PresentMissionBriefing()
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[BRIEFING_SIM_NOT_READY] Mission briefing is unavailable."));
        return;
    }
    ClearSelection();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bTitleScreenVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = false;
    bCampaignResult = false;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    bMissionBriefingVisible = true;
    Bridge->SetScenarioPaused(true);
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    const bool bPrologue =
        Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue;
    const bool bSevenAccounts =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignSevenAccounts;
    const bool bCityReserve =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignCityReserve;
    const bool bUnburiedRoad =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignUnburiedRoad;
    const bool bTermsOfContinuance =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignTermsOfContinuance;
    const bool bNamesWithoutBirths =
        Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNamesWithoutBirths;
    SetStatusMessage(
        bPrologue
            ? TEXT("WHAT THE LEDGER KEEPS — recover the archive, decide the Well, and withdraw. Enter deploys Mara Vey.")
        : bSevenAccounts
            ? TEXT("SEVEN ACCOUNTS OF RAIN — migrate the Waystone, then bring Oruun to the inherited account. Enter deploys.")
        : bCityReserve
            ? TEXT("A CITY ON RESERVE — reconnect three ark-city districts in the inherited priority order. Enter deploys Mara Vey.")
        : bUnburiedRoad
            ? TEXT("THE UNBURIED ROAD — root the Waystone, raise a Listening Spine, and recover the missing shard. Enter deploys Oruun.")
        : bTermsOfContinuance
            ? TEXT("TERMS OF CONTINUANCE — synchronize both treaty proxies, hold the fixed window, then extract both witness proxies. Enter deploys Meridian authority.")
        : bNamesWithoutBirths
            ? TEXT("NAMES WITHOUT BIRTHS — Talar must locate the inherited census trace, a worker must power its archive, both civilian proxies must reach shelter, and Talar must extract the evidence. Enter deploys Meridian authority.")
            : TEXT("GLASS SCAR OPERATIONS BRIEF — Tab changes faction; Enter deploys."),
        3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_BRIEFING_READY] operation=%s paused=true keyboardStart=true factionChoice=%s"),
        bPrologue ? TEXT("WhatTheLedgerKeeps")
        : bSevenAccounts ? TEXT("SevenAccountsOfRain")
        : bCityReserve ? TEXT("ACityOnReserve")
        : bUnburiedRoad ? TEXT("TheUnburiedRoad")
        : bTermsOfContinuance ? TEXT("TermsOfContinuance")
        : bNamesWithoutBirths ? TEXT("NamesWithoutBirths")
        : TEXT("GlassScar"),
        (bPrologue || bSevenAccounts || bCityReserve || bUnburiedRoad ||
         bTermsOfContinuance || bNamesWithoutBirths)
            ? TEXT("false")
            : TEXT("true"));
}

void AEchoesPlayerController::ConfirmMissionBriefing()
{
    if (!bMissionBriefingVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[BRIEFING_SIM_NOT_READY] Deployment could not begin."));
        return;
    }
    bMissionBriefingVisible = false;
    Bridge->SetScenarioPaused(false);
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
    if (Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue)
    {
        SetStatusMessage(TEXT("DEPLOYED — select Mara Vey's scout carrier and recover the archive at tile 22,18."), 8.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignSevenAccounts)
    {
        const FEchoesSevenAccountsRoute Route = Bridge->GetSevenAccountsRoute();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — uproot and re-root the Waystone at %d,%d; then bring Oruun to %d,%d."),
                Route.WaystoneAnchor.x.FloorToInt(),
                Route.WaystoneAnchor.y.FloorToInt(),
                Route.MemoryAccountSite.x.FloorToInt(),
                Route.MemoryAccountSite.y.FloorToInt()),
            10.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignCityReserve)
    {
        const FEchoesCityReserveGrid Grid = Bridge->GetCityReserveGrid();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — build Power Links until %s, %s, and %s district posts are powered."),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Priority),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Secondary),
                FEchoesCityReserveMissionModel::DistrictDisplayName(
                    Grid.Final)),
            12.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignUnburiedRoad)
    {
        const FEchoesUnburiedRoadRoute Route = Bridge->GetUnburiedRoadRoute();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — root the Waystone at %d,%d; build a Listening Spine at %d,%d; bring Oruun to the shard at %d,%d."),
                Route.Roadhead.x.FloorToInt(),
                Route.Roadhead.y.FloorToInt(),
                Route.ListeningSpineSite.x.FloorToInt(),
                Route.ListeningSpineSite.y.FloorToInt(),
                Route.MemoryShardSite.x.FloorToInt(),
                Route.MemoryShardSite.y.FloorToInt()),
            14.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignTermsOfContinuance)
    {
        const FEchoesTermsOfContinuancePlan Plan =
            Bridge->GetTermsOfContinuancePlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — sync Meridian proxies %d,%d + %d,%d by T%llu; hold to T%llu; extract witnesses at %d,%d."),
                Plan.MeridianRelaySite.x.FloorToInt(),
                Plan.MeridianRelaySite.y.FloorToInt(),
                Plan.KharuunSpineSite.x.FloorToInt(),
                Plan.KharuunSpineSite.y.FloorToInt(),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowStartTick),
                static_cast<unsigned long long>(
                    Plan.ContinuanceWindowEndTick),
                Plan.WitnessExtractionSite.x.FloorToInt(),
                Plan.WitnessExtractionSite.y.FloorToInt()),
            16.0f);
    }
    else if (Bridge->GetOperationMode() ==
             EEchoesOperationMode::CampaignNamesWithoutBirths)
    {
        const FEchoesNamesWithoutBirthsPlan Plan =
            Bridge->GetNamesWithoutBirthsPlan();
        SetStatusMessage(
            FString::Printf(
                TEXT("DEPLOYED — bring Talar to census %d,%d; build its Power Link at %d,%d; shelter both civilians at %d,%d; extract Talar at %d,%d."),
                Plan.CensusSite.x.FloorToInt(),
                Plan.CensusSite.y.FloorToInt(),
                Plan.PowerLinkSite.x.FloorToInt(),
                Plan.PowerLinkSite.y.FloorToInt(),
                Plan.CivilianShelterSite.x.FloorToInt(),
                Plan.CivilianShelterSite.y.FloorToInt(),
                Plan.EvidenceExtractionSite.x.FloorToInt(),
                Plan.EvidenceExtractionSite.y.FloorToInt()),
            16.0f);
    }
    else
    {
        SetStatusMessage(
            FString::Printf(
                  TEXT("DEPLOYED — secure the Future Well or destroy the %s Command Core."),
                  *GetOpponentFactionLabel()),
            8.0f);
    }
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_BRIEFING_DISMISSED] paused=false"));
}

void AEchoesPlayerController::CyclePlayableFaction()
{
    if (!bTitleScreenVisible && !bMissionBriefingVisible)
    {
        CycleOwnedEntity(1);
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[FACTION_SIM_NOT_READY] Faction choice is unavailable."));
        return;
    }
    if (Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: What the Ledger Keeps follows Mara Vey and the Meridian Compact."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignSevenAccounts)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Seven Accounts of Rain follows Oruun and the Kharuun Assemblies."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignCityReserve)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: A City on Reserve follows Mara Vey and the Meridian Compact."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignUnburiedRoad)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: The Unburied Road follows Oruun and the Kharuun Assemblies."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignTermsOfContinuance)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Terms of Continuance uses Meridian-authoritative treaty and witness proxies; mixed-faction command is not implemented."));
        return;
    }
    if (Bridge->GetOperationMode() ==
        EEchoesOperationMode::CampaignNamesWithoutBirths)
    {
        SetStatusMessage(TEXT("FACTION LOCKED: Names Without Births uses Meridian-authoritative Talar and civilian proxies."));
        return;
    }
    const echoes::sim::Faction NewFaction =
        Bridge->GetLocalFaction() == echoes::sim::Faction::MeridianCompact
            ? echoes::sim::Faction::KharuunAssemblies
            : echoes::sim::Faction::MeridianCompact;
    FString Feedback;
    if (!Bridge->SelectLocalFaction(NewFaction, Feedback))
    {
        SetStatusMessage(Feedback);
        return;
    }
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    Bridge->SetScenarioPaused(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("FACTION SELECTED: %s — opposition: %s. Press Enter when ready."),
            *GetLocalFactionLabel(),
            *GetOpponentFactionLabel()),
        3600.0f);
}

void AEchoesPlayerController::CycleOperation()
{
    if (!bTitleScreenVisible && !bMissionBriefingVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[OPERATION_SIM_NOT_READY] Operation choice is unavailable."));
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    EEchoesOperationMode NewOperation = EEchoesOperationMode::Skirmish;
    if (Bridge->GetOperationMode() == EEchoesOperationMode::Skirmish)
    {
        NewOperation = EEchoesOperationMode::CampaignPrologue;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignPrologue &&
             Bridge->IsSevenAccountsUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignSevenAccounts;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignSevenAccounts &&
             Bridge->IsCityReserveUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignCityReserve;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignCityReserve &&
             Bridge->IsUnburiedRoadUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignUnburiedRoad;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignUnburiedRoad &&
             Bridge->IsTermsOfContinuanceUnlocked())
    {
        NewOperation =
            EEchoesOperationMode::CampaignTermsOfContinuance;
    }
    else if (Bridge->GetOperationMode() ==
                 EEchoesOperationMode::CampaignTermsOfContinuance &&
             Bridge->IsNamesWithoutBirthsUnlocked())
    {
        NewOperation = EEchoesOperationMode::CampaignNamesWithoutBirths;
    }
    FString Feedback;
    if (!Bridge->SelectOperationMode(NewOperation, Feedback))
    {
        SetStatusMessage(Feedback);
        return;
    }
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    Bridge->SetScenarioPaused(true);
    SetStatusMessage(
        FString::Printf(
            TEXT("%s Press Enter when ready."),
            *Feedback),
        3600.0f);
}

bool AEchoesPlayerController::IsNewCampaignConfirmationArmed() const
{
    return bNewCampaignConfirmationArmed && GetWorld() != nullptr &&
           GetWorld()->GetTimeSeconds() <=
               NewCampaignConfirmationExpiresAt;
}

void AEchoesPlayerController::RequestNewCampaign()
{
    if (!bTitleScreenVisible)
    {
        SetStatusMessage(TEXT("[NEW_CAMPAIGN_TITLE_REQUIRED] Return to the title screen before replacing campaign progress."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[NEW_CAMPAIGN_SIM_NOT_READY] Campaign reset is unavailable."));
        return;
    }
    if (Bridge->GetCampaignProgress().Decisions.IsEmpty())
    {
        bNewCampaignConfirmationArmed = false;
        NewCampaignConfirmationExpiresAt = 0.0;
        SetStatusMessage(TEXT("NEW CAMPAIGN: the campaign ledger is already empty."));
        return;
    }
    if (!IsNewCampaignConfirmationArmed())
    {
        bNewCampaignConfirmationArmed = true;
        NewCampaignConfirmationExpiresAt =
            GetWorld()->GetTimeSeconds() + 10.0;
        SetStatusMessage(
            TEXT("NEW CAMPAIGN ARMED — press F10 again within 10 seconds to replace active progress. One prior ledger generation will be retained as backup."),
            10.0f);
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_NEW_CAMPAIGN_ARMED] records=%d confirmationSeconds=10 backupRetained=true"),
            Bridge->GetCampaignProgress().Decisions.Num());
        return;
    }

    FString Feedback;
    if (!Bridge->StartNewCampaign(Feedback))
    {
        bNewCampaignConfirmationArmed = false;
        NewCampaignConfirmationExpiresAt = 0.0;
        SetStatusMessage(Feedback, 12.0f);
        return;
    }
    bNewCampaignConfirmationArmed = false;
    NewCampaignConfirmationExpiresAt = 0.0;
    ClearSelection();
    ClearControlGroups();
    bSelectionButtonDown = false;
    bControlGroupAssignmentArmed = false;
    bCampaignResult = false;
    bCampaignSuccess = false;
    CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
    CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
    SetStatusMessage(Feedback, 12.0f);
}

void AEchoesPlayerController::CycleOwnedEntityPrevious()
{
    if (bTitleScreenVisible || bMissionBriefingVisible)
    {
        CyclePlayableFaction();
        return;
    }
    CycleOwnedEntity(-1);
}

void AEchoesPlayerController::SelectCombatForce()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Combat-force selection is unavailable."));
        return;
    }

    TArray<uint32> CombatIds;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        const bool bCombatUnit =
            Entity.type == echoes::sim::EntityType::Soldier ||
            Entity.type == echoes::sim::EntityType::HeavyUnit ||
            Entity.type == echoes::sim::EntityType::ScoutUnit;
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.hitPoints > 0 && bCombatUnit &&
            !Entity.temporaryMineralCover &&
            Bridge->FindEntityView(Entity.id) != nullptr)
        {
            CombatIds.Add(Entity.id);
        }
    }
    CombatIds.Sort();
    if (CombatIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_COMBAT_FORCE] No live owned combat unit is visible."));
        return;
    }

    ClearSelection();
    for (const uint32 EntityId : CombatIds)
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }
    SetStatusMessage(
        FString::Printf(
            TEXT("COMBAT FORCE: %d visible owned units selected // End centers force"),
            CombatIds.Num()),
        5.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_FORCE_SELECT] count=%d source=owned_presentation_views hiddenStateRead=false"),
        CombatIds.Num());
}

void AEchoesPlayerController::CycleFormation()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    switch (CurrentFormation)
    {
        case EEchoesFormationType::Box:
            CurrentFormation = EEchoesFormationType::Line;
            break;
        case EEchoesFormationType::Line:
            CurrentFormation = EEchoesFormationType::Wedge;
            break;
        case EEchoesFormationType::Wedge:
            CurrentFormation = EEchoesFormationType::Box;
            break;
    }
    SetStatusMessage(FString::Printf(
        TEXT("FORMATION: %s — Move, Attack-move, and Patrol will align to the destination."),
        *GetFormationLabel()),
        5.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_FORMATION_SELECTED] type=%s commandAuthority=destinations_only replaySafe=true"),
        *GetFormationLabel());
}

void AEchoesPlayerController::ToggleKeyboardTargeting()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    bKeyboardTargetingEnabled = !bKeyboardTargetingEnabled;
    KeyboardTargetOffset = FVector2D::ZeroVector;
    SetStatusMessage(
        bKeyboardTargetingEnabled
            ? TEXT("KEYBOARD TARGET: arrows move reticle // Space orders // F/B/N/M/F6 use reticle // Home exits")
            : TEXT("POINTER TARGET: cursor-directed orders restored."),
        6.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_TARGET_MODE] enabled=%s source=screen_reticle offsetPx=(0,0) hiddenStateRead=false"),
        bKeyboardTargetingEnabled ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::NudgeKeyboardTarget(const FVector2D& Direction)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    GetViewportSize(ViewportWidth, ViewportHeight);
    if (ViewportWidth <= 0 || ViewportHeight <= 0)
    {
        return;
    }
    bKeyboardTargetingEnabled = true;
    constexpr float StepPixels = 64.0f;
    constexpr float EdgeMarginPixels = 32.0f;
    KeyboardTargetOffset += Direction * StepPixels;
    KeyboardTargetOffset.X = FMath::Clamp(
        KeyboardTargetOffset.X,
        -(static_cast<float>(ViewportWidth) * 0.5f - EdgeMarginPixels),
        static_cast<float>(ViewportWidth) * 0.5f - EdgeMarginPixels);
    KeyboardTargetOffset.Y = FMath::Clamp(
        KeyboardTargetOffset.Y,
        -(static_cast<float>(ViewportHeight) * 0.5f - EdgeMarginPixels),
        static_cast<float>(ViewportHeight) * 0.5f - EdgeMarginPixels);
    SetStatusMessage(
        FString::Printf(
            TEXT("KEYBOARD TARGET: offset (%+.0f, %+.0f) px // Space orders // Home resets/exits"),
            KeyboardTargetOffset.X,
            KeyboardTargetOffset.Y),
        2.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_TARGET_NUDGE] offsetPx=(%.0f,%.0f) source=screen_reticle hiddenStateRead=false"),
        KeyboardTargetOffset.X,
        KeyboardTargetOffset.Y);
}

void AEchoesPlayerController::NudgeKeyboardTargetLeft()
{
    NudgeKeyboardTarget(FVector2D(-1.0f, 0.0f));
}

void AEchoesPlayerController::NudgeKeyboardTargetRight()
{
    NudgeKeyboardTarget(FVector2D(1.0f, 0.0f));
}

void AEchoesPlayerController::SnapKeyboardTargetToSelection()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[SNAP_REQUIRES_SELECTION] Select one or more visible owned entities first."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    APawn* CameraPawn = GetPawn();
    if (Bridge == nullptr || CameraPawn == nullptr)
    {
        SetStatusMessage(TEXT("[SELECTED_VIEW_UNAVAILABLE] The selected presentation views are unavailable."));
        return;
    }

    FVector Centroid = FVector::ZeroVector;
    int32 VisibleSelectionCount = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        const AEchoesEntityView* View = Bridge->FindEntityView(EntityId);
        if (Entity == nullptr ||
            Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId ||
            Entity->hitPoints <= 0 || View == nullptr)
        {
            continue;
        }
        Centroid += View->GetActorLocation();
        ++VisibleSelectionCount;
    }
    if (VisibleSelectionCount == 0)
    {
        SetStatusMessage(TEXT("[SELECTED_VIEW_UNAVAILABLE] No selected owned presentation view is available."));
        return;
    }
    Centroid /= static_cast<float>(VisibleSelectionCount);
    FVector CameraLocation = CameraPawn->GetActorLocation();
    CameraLocation.X = Centroid.X;
    CameraLocation.Y = Centroid.Y;
    CameraPawn->SetActorLocation(CameraLocation);
    KeyboardTargetOffset = FVector2D::ZeroVector;
    bKeyboardTargetingEnabled = true;
    SetStatusMessage(
        TEXT("KEYBOARD TARGET: camera and reticle centered on selected visible force // arrows choose a visible destination."),
        4.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_TARGET_SNAP] count=%d centroid=(%.1f,%.1f) offsetPx=(0,0) cameraCentered=true source=selected_owned_views hiddenStateRead=false"),
        VisibleSelectionCount,
        Centroid.X,
        Centroid.Y);
}

void AEchoesPlayerController::CycleOwnedEntity(int32 Direction)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Keyboard selection is unavailable."));
        return;
    }

    TArray<uint32> Candidates;
    for (const echoes::sim::Entity& Entity : Simulation->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.hitPoints > 0 && !Entity.temporaryMineralCover &&
            Bridge->FindEntityView(Entity.id) != nullptr)
        {
            Candidates.Add(Entity.id);
        }
    }
    Candidates.Sort();
    if (Candidates.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_OWNED_ENTITIES] No live owned entity can be selected."));
        return;
    }

    int32 CandidateIndex = Direction < 0 ? Candidates.Num() - 1 : 0;
    if (SelectedEntityIds.Num() == 1)
    {
        const int32 CurrentIndex = Candidates.IndexOfByKey(SelectedEntityIds[0]);
        if (CurrentIndex != INDEX_NONE)
        {
            CandidateIndex =
                (CurrentIndex + (Direction < 0 ? -1 : 1) + Candidates.Num()) %
                Candidates.Num();
        }
    }

    ClearSelection();
    const uint32 SelectedId = Candidates[CandidateIndex];
    SelectedEntityIds.Add(SelectedId);
    SetEntitySelected(SelectedId, true);
    const AEchoesEntityView* View = Bridge->FindEntityView(SelectedId);
    SetStatusMessage(
        FString::Printf(
            TEXT("KEYBOARD SELECT: %s  //  entity %u  //  Tab next / Backspace previous"),
            View != nullptr ? *View->GetDisplayName() : TEXT("owned entity"),
            SelectedId),
        4.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_KEYBOARD_SELECTION] entity=%u index=%d total=%d direction=%s owned=true"),
        SelectedId,
        CandidateIndex,
        Candidates.Num(),
        Direction < 0 ? TEXT("previous") : TEXT("next"));
}

void AEchoesPlayerController::ConfirmPrimaryAction()
{
    if (bTitleScreenVisible)
    {
        ConfirmTitleScreen();
    }
    else if (bMissionBriefingVisible)
    {
        ConfirmMissionBriefing();
    }
    else if (bMatchResultVisible)
    {
        RestartScenario();
    }
    else if (bTechnologyPanelVisible)
    {
        ResearchTechnologyByTier(TechnologyPanelFocusedTier);
    }
    else if (bPauseMenuVisible)
    {
        TogglePauseMenu();
    }
}

void AEchoesPlayerController::NotifyRuntimeFailure(const FString& FailureCode)
{
    bRuntimeStateKnown = true;
    SetStatusMessage(
        FString::Printf(
            TEXT("[%s] Runtime prototype initialization failed; inspect LogEchoes."),
            *FailureCode),
        15.0f);
}

void AEchoesPlayerController::NotifyMatchFinished(
    echoes::sim::MatchOutcome Outcome)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = false;
    PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
    PresentedMatchOutcome = Outcome;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString Message =
        TEXT("DRAW — both Command Cores fell in the same deterministic tick. Press R to restart.");
    if (Outcome == echoes::sim::MatchOutcome::Player0Victory)
    {
        Message =
            TEXT("VICTORY — the opposing Command Core has fallen. Press R to restart.");
    }
    else if (Outcome == echoes::sim::MatchOutcome::Player1Victory ||
             Outcome == echoes::sim::MatchOutcome::Player2Victory ||
             Outcome == echoes::sim::MatchOutcome::Player3Victory)
    {
        Message =
            TEXT("DEFEAT — your Command Core has fallen. Press R to restart.");
    }
    SetStatusMessage(Message, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_RESULT_PRESENTED] outcome=%u keyboardRestart=true"),
        static_cast<uint8>(Outcome));
}

void AEchoesPlayerController::NotifyCampaignPrologueFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    PresentedCampaignOperation = EEchoesOperationMode::CampaignPrologue;
    bCampaignSuccess = bSuccess;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — the archive carrier or withdrawal line was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — archive recovered, %s protocol completed, and Mara Vey withdrew to Lume Reach."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this decision. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::ReplayConflict)
        {
            ResultMessage += TEXT(" Replay choice retained for this result; the original campaign decision remains unchanged. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=WhatTheLedgerKeeps success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifySevenAccountsFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignSevenAccounts;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the Waystone, the local Core, or the migration route was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — the %s route is rooted and Oruun reached the matching account."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this route. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=SevenAccountsOfRain success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyCityReserveFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignCityReserve;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — a reserve district, the local Core, or the grid line was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — all three ark-city districts are powered under the inherited %s reserve plan."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this grid result. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=ACityOnReserve success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyUnburiedRoadFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignUnburiedRoad;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Oruun, the Waystone, the Listening Spine, the local Core, or the unburied route was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — Oruun recovered the missing shard beyond the inherited %s route."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this recovery. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TheUnburiedRoad success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyTermsOfContinuanceFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignTermsOfContinuance;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — a witness, network, local Core, or the continuance window was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — both witness proxies survived generic unresolved pressure and extracted under the inherited %s accord."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus ==
                 EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this continuance result. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=TermsOfContinuance success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::NotifyNamesWithoutBirthsFinished(
    bool bSuccess,
    echoes::sim::FutureWellChoice Consequence,
    echoes::sim::FutureWellChoice RecordedConsequence,
    EEchoesCampaignCommitStatus CommitStatus)
{
    ClearSelection();
    bControlGroupAssignmentArmed = false;
    bSelectionButtonDown = false;
    bTitleScreenVisible = false;
    bMissionBriefingVisible = false;
    bPauseMenuVisible = false;
    bTechnologyPanelVisible = false;
    bMatchResultVisible = true;
    bCampaignResult = true;
    bCampaignSuccess = bSuccess;
    PresentedCampaignOperation =
        EEchoesOperationMode::CampaignNamesWithoutBirths;
    CampaignConsequence = Consequence;
    RecordedCampaignConsequence = RecordedConsequence;
    CampaignCommitStatus = CommitStatus;
    FutureWellChoice = Consequence;
    PresentedMatchOutcome = bSuccess
        ? echoes::sim::MatchOutcome::Player0Victory
        : echoes::sim::MatchOutcome::Player1Victory;
    SetIgnoreMoveInput(true);
    SetIgnoreLookInput(true);
    FString ResultMessage =
        TEXT("MISSION FAILED — Talar, the census archive, a civilian proxy, the local Core, or the operation was lost. Press R to replay.");
    if (bSuccess)
    {
        ResultMessage = FString::Printf(
            TEXT("MISSION COMPLETE — Talar extracted the %s census trace after the archive was powered and both civilian proxies reached shelter."),
            *GetFutureWellChoiceLabel());
        if (CommitStatus == EEchoesCampaignCommitStatus::Added)
        {
            ResultMessage += TEXT(" Campaign ledger committed. Press R to replay.");
        }
        else if (CommitStatus == EEchoesCampaignCommitStatus::AlreadyRecorded)
        {
            ResultMessage += TEXT(" Campaign ledger already contains this census recovery. Press R to replay.");
        }
        else
        {
            ResultMessage += TEXT(" Campaign progress was not saved. Press R to replay.");
        }
    }
    SetStatusMessage(ResultMessage, 3600.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_CAMPAIGN_RESULT_PRESENTED] mission=NamesWithoutBirths success=%s consequence=%u recordedConsequence=%u campaignStatus=%u keyboardRestart=true"),
        bSuccess ? TEXT("true") : TEXT("false"),
        static_cast<uint8>(Consequence),
        static_cast<uint8>(RecordedConsequence),
        static_cast<uint8>(CommitStatus));
}

void AEchoesPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (bSelectionButtonDown)
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        if (GetMousePosition(MouseX, MouseY))
        {
            SelectionCurrentScreenPosition = FVector2D(MouseX, MouseY);
        }
    }
    PruneSelection();
    if (bControlGroupAssignmentArmed &&
        GetWorld() != nullptr &&
        GetWorld()->GetTimeSeconds() > ControlGroupAssignmentExpiresAt)
    {
        bControlGroupAssignmentArmed = false;
    }
}

void AEchoesPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    check(InputComponent != nullptr);

    InputComponent->BindAction(
        TEXT("Select"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::SelectionPressed);
    InputComponent->BindAction(
        TEXT("Select"),
        IE_Released,
        this,
        &AEchoesPlayerController::SelectionReleased);
    InputComponent->BindAction(
        TEXT("ContextOrder"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ContextOrderPressed);
    InputComponent->BindAction(
        TEXT("ChooseHarvest"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseHarvest);
    InputComponent->BindAction(
        TEXT("ChoosePreserve"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChoosePreserve);
    InputComponent->BindAction(
        TEXT("ChooseReshape"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ChooseReshape);
    InputComponent->BindAction(
        TEXT("BuildBarracks"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildBarracks);
    InputComponent->BindAction(
        TEXT("BuildDropoff"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildDropoff);
    InputComponent->BindAction(
        TEXT("BuildUtility"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::BuildUtility);
    InputComponent->BindAction(
        TEXT("ProduceWorker"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceWorker);
    InputComponent->BindAction(
        TEXT("ProduceSoldier"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceSoldier);
    InputComponent->BindAction(
        TEXT("ProduceHeavy"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceHeavy);
    InputComponent->BindAction(
        TEXT("ProduceScout"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ProduceScout);
    InputComponent->BindAction(
        TEXT("ResearchNext"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ResearchNextTechnology);
    InputComponent->BindAction(
        TEXT("ToggleTechnologyPanel"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ToggleTechnologyPanel);
    InputComponent->BindAction(
        TEXT("TechnologyFocusPrevious"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::FocusPreviousTechnologyTier);
    InputComponent->BindAction(
        TEXT("TechnologyFocusNext"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::FocusNextTechnologyTier);
    InputComponent->BindAction(
        TEXT("AttackMoveAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AttackMoveAtCursor);
    InputComponent->BindAction(
        TEXT("PatrolAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::PatrolAtCursor);
    InputComponent->BindAction(
        TEXT("HoldSelected"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::HoldSelectedUnits);
    InputComponent->BindAction(
        TEXT("GuardAtCursor"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::GuardAtCursor);
    InputComponent->BindAction(
        TEXT("StopSelected"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::StopSelectedUnits);
    InputComponent->BindAction(
        TEXT("ToggleBulwarkDeployment"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ToggleBulwarkDeploymentAtCursor);
    InputComponent->BindAction(
        TEXT("ActivateRelaySupply"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ActivateRelaySupply);
    InputComponent->BindAction(
        TEXT("ToggleWaystoneRoot"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::ToggleWaystoneRoot);
    InputComponent->BindAction(
        TEXT("AdaptWarformCarapace"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AdaptSelectedWarformsCarapace);
    InputComponent->BindAction(
        TEXT("AdaptWarformStriker"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::AdaptSelectedWarformsStriker);
    InputComponent->BindAction(
        TEXT("RaiseMineralCover"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::RaiseSelectedCairnbackCoverAtCursor);
    InputComponent->BindAction(
        TEXT("PauseScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::TogglePauseMenu);
    InputComponent->BindAction(
        TEXT("RestartScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::RestartScenario);
    InputComponent->BindAction(
        TEXT("QuickSaveScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::QuickSaveScenario);
    InputComponent->BindAction(
        TEXT("QuickLoadScenario"),
        IE_Pressed,
        this,
        &AEchoesPlayerController::QuickLoadScenario);
    const auto BindPressed = [this](
                                 const FName ActionName,
                                 void (AEchoesPlayerController::*Handler)())
    {
        InputComponent->BindAction(ActionName, IE_Pressed, this, Handler);
    };
    BindPressed(
        TEXT("ArmControlGroupAssignment"),
        &AEchoesPlayerController::ArmControlGroupAssignment);
    BindPressed(TEXT("CycleHudScale"), &AEchoesPlayerController::CycleHudScale);
    BindPressed(TEXT("ToggleHighContrast"), &AEchoesPlayerController::ToggleHighContrast);
    BindPressed(TEXT("ToggleReducedMotion"), &AEchoesPlayerController::ToggleReducedMotion);
    BindPressed(TEXT("ToggleReducedFlashing"), &AEchoesPlayerController::ToggleReducedFlashing);
    BindPressed(TEXT("ToggleEdgePan"), &AEchoesPlayerController::ToggleEdgePan);
    BindPressed(TEXT("DecreaseCameraPanSpeed"), &AEchoesPlayerController::DecreaseCameraPanSpeed);
    BindPressed(TEXT("IncreaseCameraPanSpeed"), &AEchoesPlayerController::IncreaseCameraPanSpeed);
    BindPressed(TEXT("DecreaseCameraZoomSpeed"), &AEchoesPlayerController::DecreaseCameraZoomSpeed);
    BindPressed(TEXT("IncreaseCameraZoomSpeed"), &AEchoesPlayerController::IncreaseCameraZoomSpeed);
    BindPressed(TEXT("ConfirmPrimaryAction"), &AEchoesPlayerController::ConfirmPrimaryAction);
    BindPressed(TEXT("CyclePlayableFaction"), &AEchoesPlayerController::CyclePlayableFaction);
    BindPressed(TEXT("CycleOperation"), &AEchoesPlayerController::CycleOperation);
    BindPressed(TEXT("RequestNewCampaign"), &AEchoesPlayerController::RequestNewCampaign);
    BindPressed(TEXT("CycleOwnedEntityPrevious"), &AEchoesPlayerController::CycleOwnedEntityPrevious);
    BindPressed(TEXT("SelectCombatForce"), &AEchoesPlayerController::SelectCombatForce);
    BindPressed(TEXT("CycleFormation"), &AEchoesPlayerController::CycleFormation);
    BindPressed(TEXT("ToggleKeyboardTargeting"), &AEchoesPlayerController::ToggleKeyboardTargeting);
    BindPressed(TEXT("KeyboardContextOrder"), &AEchoesPlayerController::KeyboardContextOrderPressed);
    BindPressed(TEXT("KeyboardTargetLeft"), &AEchoesPlayerController::NudgeKeyboardTargetLeft);
    BindPressed(TEXT("KeyboardTargetRight"), &AEchoesPlayerController::NudgeKeyboardTargetRight);
    BindPressed(TEXT("SnapKeyboardTargetToSelection"), &AEchoesPlayerController::SnapKeyboardTargetToSelection);
    BindPressed(TEXT("RecallControlGroup1"), &AEchoesPlayerController::RecallControlGroup1);
    BindPressed(TEXT("RecallControlGroup2"), &AEchoesPlayerController::RecallControlGroup2);
    BindPressed(TEXT("RecallControlGroup3"), &AEchoesPlayerController::RecallControlGroup3);
    BindPressed(TEXT("RecallControlGroup4"), &AEchoesPlayerController::RecallControlGroup4);
    BindPressed(TEXT("RecallControlGroup5"), &AEchoesPlayerController::RecallControlGroup5);
    BindPressed(TEXT("RecallControlGroup6"), &AEchoesPlayerController::RecallControlGroup6);
    BindPressed(TEXT("RecallControlGroup7"), &AEchoesPlayerController::RecallControlGroup7);
    BindPressed(TEXT("RecallControlGroup8"), &AEchoesPlayerController::RecallControlGroup8);
    BindPressed(TEXT("RecallControlGroup9"), &AEchoesPlayerController::RecallControlGroup9);
    BindPressed(TEXT("RecallControlGroup0"), &AEchoesPlayerController::RecallControlGroup0);
}

void AEchoesPlayerController::SelectionPressed()
{
    if (bTechnologyPanelVisible)
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        if (GetMousePosition(MouseX, MouseY))
        {
            (void)HandleTechnologyPanelPointer(FVector2D(MouseX, MouseY));
        }
        return;
    }
    if (IsModalOverlayVisible())
    {
        return;
    }
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        SetStatusMessage(TEXT("[CURSOR_UNAVAILABLE] Selection could not read the pointer position."));
        return;
    }
    SelectionStartScreenPosition = FVector2D(MouseX, MouseY);
    SelectionCurrentScreenPosition = SelectionStartScreenPosition;
    bSelectionButtonDown = true;
}

void AEchoesPlayerController::SelectionReleased()
{
    if (IsModalOverlayVisible())
    {
        bSelectionButtonDown = false;
        return;
    }
    if (!bSelectionButtonDown)
    {
        return;
    }

    float MouseX = SelectionCurrentScreenPosition.X;
    float MouseY = SelectionCurrentScreenPosition.Y;
    if (GetMousePosition(MouseX, MouseY))
    {
        SelectionCurrentScreenPosition = FVector2D(MouseX, MouseY);
    }
    bSelectionButtonDown = false;

    const bool bAdditive = IsInputKeyDown(EKeys::LeftShift) ||
                           IsInputKeyDown(EKeys::RightShift);
    if (FVector2D::Distance(
            SelectionStartScreenPosition,
            SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels)
    {
        SelectInScreenRectangle(bAdditive);
    }
    else
    {
        SelectAtCursor(bAdditive);
    }
}

void AEchoesPlayerController::SelectAtCursor(bool bAdditive)
{
    FHitResult HitResult;
    AEchoesEntityView* View = nullptr;
    if (TraceCursor(HitResult))
    {
        View = Cast<AEchoesEntityView>(HitResult.GetActor());
    }

    if (View == nullptr ||
        View->GetOwnerPlayerId() != UEchoesSimulationSubsystem::LocalPlayerId)
    {
        if (!bAdditive)
        {
            ClearSelection();
        }
        return;
    }

    const uint32 EntityId = View->GetEntityId();
    if (!bAdditive)
    {
        ClearSelection();
    }

    if (bAdditive && SelectedEntityIds.Contains(EntityId))
    {
        SetEntitySelected(EntityId, false);
        SelectedEntityIds.Remove(EntityId);
    }
    else if (!SelectedEntityIds.Contains(EntityId))
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Selected %d owned entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
}

void AEchoesPlayerController::SelectInScreenRectangle(bool bAdditive)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Sim =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Sim == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Drag selection is unavailable."));
        return;
    }

    if (!bAdditive)
    {
        ClearSelection();
    }

    const float MinX = FMath::Min(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MaxX = FMath::Max(
        SelectionStartScreenPosition.X,
        SelectionCurrentScreenPosition.X);
    const float MinY = FMath::Min(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);
    const float MaxY = FMath::Max(
        SelectionStartScreenPosition.Y,
        SelectionCurrentScreenPosition.Y);

    for (const echoes::sim::Entity& Entity : Sim->Entities())
    {
        if (Entity.owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            continue;
        }
        AEchoesEntityView* View = Bridge->FindEntityView(Entity.id);
        if (View == nullptr)
        {
            continue;
        }

        FVector2D ScreenPosition;
        if (ProjectWorldLocationToScreen(
                View->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f),
                ScreenPosition,
                false) &&
            ScreenPosition.X >= MinX && ScreenPosition.X <= MaxX &&
            ScreenPosition.Y >= MinY && ScreenPosition.Y <= MaxY &&
            !SelectedEntityIds.Contains(Entity.id))
        {
            SelectedEntityIds.Add(Entity.id);
            View->SetSelected(true);
        }
    }

    SetStatusMessage(
        FString::Printf(
            TEXT("Drag-selected %d owned entit%s."),
            SelectedEntityIds.Num(),
            SelectedEntityIds.Num() == 1 ? TEXT("y") : TEXT("ies")),
        2.0f);
}

void AEchoesPlayerController::ContextOrderPressed()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }

    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    FHitResult HitResult;
    if (!TraceCursor(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Point at the battlefield or an entity."));
        return;
    }

    IssueContextOrder(HitResult);
}

void AEchoesPlayerController::KeyboardContextOrderPressed()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    FHitResult HitResult;
    if (!TraceKeyboardTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_KEYBOARD_TARGET] Move the reticle until it crosses the battlefield or an entity."));
        return;
    }
    if (!bKeyboardTargetingEnabled)
    {
        bKeyboardTargetingEnabled = true;
        UE_LOG(
            LogEchoes,
            Display,
            TEXT("[ECHOES_KEYBOARD_TARGET_MODE] enabled=true source=screen_reticle hiddenStateRead=false implicit=space"));
    }
    IssueContextOrder(HitResult);
}

void AEchoesPlayerController::IssueContextOrder(const FHitResult& HitResult)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Orders cannot be issued."));
        return;
    }

    const AEchoesEntityView* TargetView =
        Cast<AEchoesEntityView>(HitResult.GetActor());
    const echoes::sim::Entity* TargetEntity =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;

    echoes::sim::CommandType CommandType = echoes::sim::CommandType::Move;
    uint32 TargetId = 0;
    FVector Destination = HitResult.Location;
    if (TargetEntity != nullptr)
    {
        TargetId = TargetEntity->id;
        Destination = Bridge->SimToWorld(TargetEntity->position);
        if (TargetEntity->type == echoes::sim::EntityType::ResourceNode)
        {
            CommandType = echoes::sim::CommandType::Gather;
        }
        else if (TargetEntity->type == echoes::sim::EntityType::FutureWell)
        {
            CommandType = echoes::sim::CommandType::FutureWell;
        }
        else if (TargetEntity->owner != echoes::sim::kNeutralPlayer &&
                 TargetEntity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            CommandType = echoes::sim::CommandType::Attack;
        }
        else if (TargetEntity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
                 (TargetEntity->type == echoes::sim::EntityType::CommandCore ||
                  TargetEntity->type == echoes::sim::EntityType::Dropoff))
        {
            CommandType = echoes::sim::CommandType::Deliver;
        }
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(Destination, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        echoes::sim::CommandType ActorCommandType = CommandType;
        uint32 ActorTargetId = TargetId;
        const echoes::sim::Entity* ActorState =
            Bridge->FindEntity(SelectedEntityIds[Index]);
        if (CommandType == echoes::sim::CommandType::Deliver &&
            (ActorState == nullptr ||
             ActorState->type != echoes::sim::EntityType::Worker ||
             ActorState->cargo <= 0))
        {
            ActorCommandType = echoes::sim::CommandType::Move;
            ActorTargetId = 0;
        }

        FVector UnitDestination = Destination;
        if (ActorCommandType == echoes::sim::CommandType::Move)
        {
            UnitDestination = FormationDestinations[Index];
        }

        FString Feedback;
        if (Bridge->IssueCommand(
                ActorCommandType,
                SelectedEntityIds[Index],
                ActorTargetId,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }

    if (AcceptedCount > 0)
    {
        const FString OrderLabel =
            CommandType == echoes::sim::CommandType::Deliver
                ? TEXT("CONTEXT MOVE / DELIVER")
                : CommandType == echoes::sim::CommandType::Move
                      ? FString::Printf(
                            TEXT("MOVE / %s"),
                            *GetFormationLabel())
                      : CommandLabel(CommandType);
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(
            FString::Printf(
                TEXT("%s: %d queued%s"),
                *OrderLabel,
                AcceptedCount,
                *RejectionSuffix));
        ShowAcceptedCommandMarker(
            Destination,
            CommandType == echoes::sim::CommandType::Attack
                ? EEchoesCommandMarkerType::AttackMove
                : EEchoesCommandMarkerType::Move,
            AcceptedCount);
    }
    else
    {
        SetStatusMessage(LastRejection.IsEmpty()
                             ? TEXT("[ORDER_REJECTED] No selected entity accepted the order.")
                             : LastRejection);
    }
}

void AEchoesPlayerController::SetEntitySelected(uint32 EntityId, bool bSelected)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge != nullptr)
    {
        if (AEchoesEntityView* View = Bridge->FindEntityView(EntityId))
        {
            View->SetSelected(bSelected);
        }
    }
}

void AEchoesPlayerController::ClearSelection()
{
    for (const uint32 EntityId : SelectedEntityIds)
    {
        SetEntitySelected(EntityId, false);
    }
    SelectedEntityIds.Reset();
}

bool AEchoesPlayerController::SetControlGroup(
    int32 GroupIndex,
    const TArray<uint32>& EntityIds,
    FString& OutFeedback)
{
    OutFeedback.Reset();
    if (GroupIndex < 0 || GroupIndex >= ControlGroupCount)
    {
        OutFeedback = TEXT("[GROUP_INDEX_INVALID] Control group must be between 0 and 9.");
        return false;
    }
    if (EntityIds.IsEmpty())
    {
        ControlGroups[GroupIndex].Reset();
        OutFeedback = FString::Printf(
            TEXT("CONTROL GROUP %d CLEARED."),
            ControlGroupDisplayNumber(GroupIndex));
        return true;
    }

    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    TArray<uint32> ValidIds;
    for (const uint32 EntityId : EntityIds)
    {
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(EntityId) : nullptr;
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId)
        {
            ValidIds.AddUnique(EntityId);
        }
    }
    if (ValidIds.IsEmpty())
    {
        OutFeedback = TEXT("[GROUP_NO_VALID_ENTITIES] No live local entities were assigned.");
        return false;
    }
    ValidIds.Sort();
    ControlGroups[GroupIndex] = MoveTemp(ValidIds);
    OutFeedback = FString::Printf(
        TEXT("CONTROL GROUP %d: %d entit%s assigned."),
        ControlGroupDisplayNumber(GroupIndex),
        ControlGroups[GroupIndex].Num(),
        ControlGroups[GroupIndex].Num() == 1 ? TEXT("y") : TEXT("ies"));
    return true;
}

TArray<uint32> AEchoesPlayerController::GetValidControlGroup(
    int32 GroupIndex) const
{
    TArray<uint32> ValidIds;
    if (GroupIndex < 0 || GroupIndex >= ControlGroupCount)
    {
        return ValidIds;
    }
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    for (const uint32 EntityId : ControlGroups[GroupIndex])
    {
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(EntityId) : nullptr;
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId)
        {
            ValidIds.Add(EntityId);
        }
    }
    return ValidIds;
}

int32 AEchoesPlayerController::ControlGroupDisplayNumber(int32 GroupIndex)
{
    return GroupIndex == ControlGroupCount - 1 ? 0 : GroupIndex + 1;
}

void AEchoesPlayerController::ClearControlGroups()
{
    for (TArray<uint32>& Group : ControlGroups)
    {
        Group.Reset();
    }
}

void AEchoesPlayerController::AssignControlGroupFromSelection(int32 GroupIndex)
{
    PruneSelection();
    FString Feedback;
    SetControlGroup(GroupIndex, SelectedEntityIds, Feedback);
    SetStatusMessage(Feedback);
}

void AEchoesPlayerController::ArmControlGroupAssignment()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    bControlGroupAssignmentArmed = true;
    ControlGroupAssignmentExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + 5.0 : 5.0;
    SetStatusMessage(
        TEXT("GROUP ASSIGNMENT ARMED — press 1-0 within five seconds."),
        5.0f);
}

void AEchoesPlayerController::RecallControlGroup(int32 GroupIndex)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    if (bControlGroupAssignmentArmed)
    {
        bControlGroupAssignmentArmed = false;
        AssignControlGroupFromSelection(GroupIndex);
        return;
    }
    TArray<uint32> ValidIds = GetValidControlGroup(GroupIndex);
    if (ValidIds.IsEmpty())
    {
        ControlGroups[GroupIndex].Reset();
        SetStatusMessage(FString::Printf(
            TEXT("[GROUP_EMPTY] Control group %d has no live entities."),
            ControlGroupDisplayNumber(GroupIndex)));
        return;
    }
    ControlGroups[GroupIndex] = ValidIds;
    ClearSelection();
    for (const uint32 EntityId : ValidIds)
    {
        SelectedEntityIds.Add(EntityId);
        SetEntitySelected(EntityId, true);
    }
    SetStatusMessage(FString::Printf(
        TEXT("CONTROL GROUP %d: %d entit%s selected."),
        ControlGroupDisplayNumber(GroupIndex),
        ValidIds.Num(),
        ValidIds.Num() == 1 ? TEXT("y") : TEXT("ies")));
}

#define DEFINE_CONTROL_GROUP_HANDLER(DisplayNumber, GroupIndex)              \
    void AEchoesPlayerController::RecallControlGroup##DisplayNumber()         \
    {                                                                         \
        RecallControlGroup(GroupIndex);                                        \
    }

DEFINE_CONTROL_GROUP_HANDLER(1, 0)
DEFINE_CONTROL_GROUP_HANDLER(2, 1)
DEFINE_CONTROL_GROUP_HANDLER(3, 2)
DEFINE_CONTROL_GROUP_HANDLER(4, 3)
DEFINE_CONTROL_GROUP_HANDLER(5, 4)
DEFINE_CONTROL_GROUP_HANDLER(6, 5)
DEFINE_CONTROL_GROUP_HANDLER(7, 6)
DEFINE_CONTROL_GROUP_HANDLER(8, 7)
DEFINE_CONTROL_GROUP_HANDLER(9, 8)
DEFINE_CONTROL_GROUP_HANDLER(0, 9)

#undef DEFINE_CONTROL_GROUP_HANDLER

void AEchoesPlayerController::PruneSelection()
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    for (int32 Index = SelectedEntityIds.Num() - 1; Index >= 0; --Index)
    {
        const echoes::sim::Entity* Entity =
            Bridge != nullptr ? Bridge->FindEntity(SelectedEntityIds[Index]) : nullptr;
        if (Entity == nullptr ||
            Entity->owner != UEchoesSimulationSubsystem::LocalPlayerId)
        {
            SelectedEntityIds.RemoveAtSwap(Index, 1, EAllowShrinking::No);
        }
    }
}

bool AEchoesPlayerController::TraceCursor(FHitResult& OutHitResult)
{
    return GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        true,
        OutHitResult);
}

bool AEchoesPlayerController::TraceKeyboardTarget(FHitResult& OutHitResult)
{
    int32 ViewportWidth = 0;
    int32 ViewportHeight = 0;
    GetViewportSize(ViewportWidth, ViewportHeight);
    if (ViewportWidth <= 0 || ViewportHeight <= 0)
    {
        return false;
    }
    return GetHitResultAtScreenPosition(
        FVector2D(
            static_cast<float>(ViewportWidth) * 0.5f,
            static_cast<float>(ViewportHeight) * 0.5f) + KeyboardTargetOffset,
        ECC_Visibility,
        true,
        OutHitResult);
}

bool AEchoesPlayerController::TraceCommandTarget(FHitResult& OutHitResult)
{
    return bKeyboardTargetingEnabled
        ? TraceKeyboardTarget(OutHitResult)
        : TraceCursor(OutHitResult);
}

TArray<FVector> AEchoesPlayerController::BuildSelectedFormationDestinations(
    const FVector& Anchor,
    int32 UnitCount)
{
    const UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FVector Centroid = FVector::ZeroVector;
    int32 PositionCount = 0;
    if (Bridge != nullptr)
    {
        for (const uint32 EntityId : SelectedEntityIds)
        {
            const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
            if (Entity != nullptr)
            {
                Centroid += Bridge->SimToWorld(Entity->position);
                ++PositionCount;
            }
        }
    }
    if (PositionCount > 0)
    {
        Centroid /= static_cast<float>(PositionCount);
        FVector NewForward = Anchor - Centroid;
        NewForward.Z = 0.0f;
        if (NewForward.SizeSquared() > 1.0f)
        {
            LastFormationForward = NewForward.GetSafeNormal();
        }
    }
    return FEchoesFormationLayout::BuildDestinations(
        Anchor,
        LastFormationForward,
        UnitCount,
        CurrentFormation,
        FormationSpacingWorldUnits);
}

void AEchoesPlayerController::ChooseHarvest()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Harvest);
}

void AEchoesPlayerController::ChoosePreserve()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Preserve);
}

void AEchoesPlayerController::ChooseReshape()
{
    SetFutureWellChoice(echoes::sim::FutureWellChoice::Reshape);
}

void AEchoesPlayerController::BuildBarracks()
{
    BuildAtCursor(echoes::sim::EntityType::Barracks);
}

void AEchoesPlayerController::BuildDropoff()
{
    BuildAtCursor(echoes::sim::EntityType::Dropoff);
}

void AEchoesPlayerController::BuildUtility()
{
    BuildAtCursor(echoes::sim::EntityType::UtilityStructure);
}

void AEchoesPlayerController::ProduceWorker()
{
    ProduceUnit(echoes::sim::EntityType::Worker);
}

void AEchoesPlayerController::ProduceSoldier()
{
    ProduceUnit(echoes::sim::EntityType::Soldier);
}

void AEchoesPlayerController::ProduceHeavy()
{
    ProduceUnit(echoes::sim::EntityType::HeavyUnit);
}

void AEchoesPlayerController::ProduceScout()
{
    ProduceUnit(echoes::sim::EntityType::ScoutUnit);
}

void AEchoesPlayerController::ResearchNextTechnology()
{
    if (IsModalOverlayVisible() && !bTechnologyPanelVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || Player == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Research is unavailable."));
        return;
    }
    const echoes::sim::ResearchType Candidates[] = {
        Player->faction == echoes::sim::Faction::MeridianCompact
            ? echoes::sim::ResearchType::MeridianPrismaticTargeting
            : echoes::sim::ResearchType::KharuunEchoCartography,
        Player->faction == echoes::sim::Faction::MeridianCompact
            ? echoes::sim::ResearchType::MeridianHorizonLattice
            : echoes::sim::ResearchType::KharuunAncestralEdge,
    };
    for (const echoes::sim::ResearchType Research : Candidates)
    {
        if (Player->HasCompletedResearch(Research))
        {
            continue;
        }
        ResearchTechnology(Research);
        return;
    }
    SetStatusMessage(TEXT("RESEARCH COMPLETE: both faction technologies are operational."));
}

void AEchoesPlayerController::ResearchTechnologyByTier(int32 TierIndex)
{
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Player == nullptr || TierIndex < 0 || TierIndex > 1)
    {
        SetStatusMessage(TEXT("[RESEARCH_TECHNOLOGY_INVALID] Technology selection is unavailable."));
        return;
    }
    const bool bMeridian =
        Player->faction == echoes::sim::Faction::MeridianCompact;
    const echoes::sim::ResearchType Research =
        TierIndex == 0
            ? (bMeridian
                   ? echoes::sim::ResearchType::MeridianPrismaticTargeting
                   : echoes::sim::ResearchType::KharuunEchoCartography)
            : (bMeridian
                   ? echoes::sim::ResearchType::MeridianHorizonLattice
                   : echoes::sim::ResearchType::KharuunAncestralEdge);
    ResearchTechnology(Research);
}

void AEchoesPlayerController::ResearchTechnology(
    echoes::sim::ResearchType Research)
{
    if (IsModalOverlayVisible() && !bTechnologyPanelVisible)
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    const echoes::sim::PlayerState* Player =
        Simulation != nullptr
            ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
            : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || Player == nullptr)
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Research is unavailable."));
        return;
    }
    if (Player->activeResearch != echoes::sim::ResearchType::None)
    {
        SetStatusMessage(TEXT("[RESEARCH_BUSY] A technology is already in progress."));
        return;
    }
    uint32 ProducerId = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity != nullptr &&
            Entity->owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity->type == echoes::sim::EntityType::Barracks)
        {
            ProducerId = EntityId;
            break;
        }
    }
    if (ProducerId == 0)
    {
        SetStatusMessage(TEXT("[RESEARCH_PRODUCER_INVALID] Select a production structure before choosing a technology."));
        return;
    }
    FString Feedback;
    if (!Bridge->IssueResearchCommand(ProducerId, Research, Feedback))
    {
        SetStatusMessage(Feedback);
        return;
    }
    const TCHAR* Label =
        Research == echoes::sim::ResearchType::MeridianPrismaticTargeting
            ? TEXT("PRISMATIC TARGETING")
            : Research == echoes::sim::ResearchType::MeridianHorizonLattice
                  ? TEXT("HORIZON LATTICE")
                  : Research == echoes::sim::ResearchType::KharuunEchoCartography
                        ? TEXT("ECHO CARTOGRAPHY")
                        : TEXT("ANCESTRAL EDGE");
    SetStatusMessage(FString::Printf(TEXT("%s: research queued."), Label));
}

void AEchoesPlayerController::AttackMoveAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Attack-move is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target an attack-move destination with the pointer or center reticle."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(HitResult.Location, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const FVector UnitDestination = FormationDestinations[Index];
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::AttackMove,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("ATTACK-MOVE / %s: %d queued%s"),
            *GetFormationLabel(),
            AcceptedCount,
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::AttackMove,
            AcceptedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[ATTACK_MOVE_REJECTED] No selected entity can attack-move.")
                : LastRejection);
    }
}

void AEchoesPlayerController::PatrolAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Patrol is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned combat units first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target a patrol endpoint with the pointer or center reticle."));
        return;
    }

    const int32 UnitCount = SelectedEntityIds.Num();
    const TArray<FVector> FormationDestinations =
        BuildSelectedFormationDestinations(HitResult.Location, UnitCount);
    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (int32 Index = 0; Index < UnitCount; ++Index)
    {
        const FVector UnitDestination = FormationDestinations[Index];
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::Patrol,
                SelectedEntityIds[Index],
                0,
                UnitDestination,
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("PATROL / %s: %d route%s assigned%s"),
            *GetFormationLabel(),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::Patrol,
            AcceptedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[PATROL_REJECTED] No selected entity can patrol.")
                : LastRejection);
    }
}

void AEchoesPlayerController::StopSelectedUnits()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Stop is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned units first."));
        return;
    }
    int32 AcceptedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Stop,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(TEXT("STOP: %d unit%s stopped."),
                              AcceptedCount,
                              AcceptedCount == 1 ? TEXT("") : TEXT("s"))
            : LastRejection.IsEmpty()
                  ? TEXT("[STOP_REJECTED] No selected entity accepted the order.")
                  : LastRejection);
}

void AEchoesPlayerController::ToggleBulwarkDeploymentAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Bulwark deployment is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target the threat direction with the pointer or center reticle."));
        return;
    }

    int32 DeployedCount = 0;
    int32 PackedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::MeridianCompact ||
            Entity->type != echoes::sim::EntityType::HeavyUnit)
        {
            ++RejectedCount;
            continue;
        }
        const bool bWasDeployed = Entity->deployed;
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::ToggleDeploy,
                EntityId,
                0,
                HitResult.Location,
                FutureWellChoice,
                Feedback))
        {
            if (bWasDeployed)
            {
                ++PackedCount;
            }
            else
            {
                ++DeployedCount;
            }
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (DeployedCount + PackedCount > 0)
    {
        SetStatusMessage(FString::Printf(
            TEXT("BULWARK: %d deploying toward cursor, %d packing, %d rejected."),
            DeployedCount,
            PackedCount,
            RejectedCount));
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[BULWARK_REQUIRED] Select a Meridian Bulwark Team.")
                : LastRejection);
    }
}

void AEchoesPlayerController::ActivateRelaySupply()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Relay supply is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::MeridianCompact ||
            Entity->type != echoes::sim::EntityType::ScoutUnit)
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::ActivateRelaySupply,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("RELAY SUPPLY: %d extension%s activated, %d rejected."),
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[RELAY_REQUIRED] Select a connected Meridian Relay Skiff.")
                  : LastRejection);
}

void AEchoesPlayerController::ToggleWaystoneRoot()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Waystone migration is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::KharuunAssemblies ||
            Entity->type != echoes::sim::EntityType::Dropoff)
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::ToggleWaystoneRoot,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("WAYSTONE: %d state change%s started, %d rejected."),
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[WAYSTONE_REQUIRED] Select a Kharuun Waystone.")
                  : LastRejection);
}

void AEchoesPlayerController::AdaptSelectedWarformsCarapace()
{
    AdaptSelectedWarforms(echoes::sim::WarformAdaptation::Carapace);
}

void AEchoesPlayerController::AdaptSelectedWarformsStriker()
{
    AdaptSelectedWarforms(echoes::sim::WarformAdaptation::Striker);
}

void AEchoesPlayerController::AdaptSelectedWarforms(
    echoes::sim::WarformAdaptation Adaptation)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    const echoes::sim::Simulation* Simulation =
        Bridge != nullptr ? Bridge->GetSimulation() : nullptr;
    if (Bridge == nullptr || Simulation == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Warform adaptation is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::KharuunAssemblies ||
            (Entity->type != echoes::sim::EntityType::Soldier &&
             Entity->type != echoes::sim::EntityType::HeavyUnit &&
             Entity->type != echoes::sim::EntityType::ScoutUnit))
        {
            ++RejectedCount;
            continue;
        }
        uint32 NearestBasin = 0;
        uint64 NearestDistance = TNumericLimits<uint64>::Max();
        for (const echoes::sim::Entity& Candidate : Simulation->Entities())
        {
            if (Candidate.owner != Entity->owner || !Candidate.completed ||
                Candidate.hitPoints <= 0 ||
                Candidate.faction != echoes::sim::Faction::KharuunAssemblies ||
                Candidate.type != echoes::sim::EntityType::Barracks)
            {
                continue;
            }
            const int64 DeltaX = static_cast<int64>(Entity->position.x.Raw()) -
                                 Candidate.position.x.Raw();
            const int64 DeltaY = static_cast<int64>(Entity->position.y.Raw()) -
                                 Candidate.position.y.Raw();
            const uint64 Distance = static_cast<uint64>(
                DeltaX * DeltaX + DeltaY * DeltaY);
            if (Distance < NearestDistance ||
                (Distance == NearestDistance && Candidate.id < NearestBasin))
            {
                NearestDistance = Distance;
                NearestBasin = Candidate.id;
            }
        }
        FString Feedback;
        if (NearestBasin != 0 && Bridge->IssueWarformAdaptation(
                EntityId, NearestBasin, Adaptation, Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = NearestBasin == 0
                ? TEXT("[GROWTH_BASIN_REQUIRED] No completed friendly Growth Basin is available.")
                : Feedback;
        }
    }
    const TCHAR* FormName =
        Adaptation == echoes::sim::WarformAdaptation::Carapace
            ? TEXT("CARAPACE")
            : TEXT("STRIKER");
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("%s MOLT: %d warform%s started, %d rejected."),
                  FormName,
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[WARFORM_REQUIRED] Select a Kharuun combat warform.")
                  : LastRejection);
}

void AEchoesPlayerController::RaiseSelectedCairnbackCoverAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Mineral cover is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target a clear cover position with the pointer or center reticle."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity == nullptr ||
            Entity->faction != echoes::sim::Faction::KharuunAssemblies ||
            Entity->type != echoes::sim::EntityType::HeavyUnit ||
            Entity->temporaryMineralCover)
        {
            ++RejectedCount;
            continue;
        }
        FString Feedback;
        if (Bridge->IssueMineralCover(EntityId, HitResult.Location, Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    SetStatusMessage(
        AcceptedCount > 0
            ? FString::Printf(
                  TEXT("MINERAL COVER: %d barrier%s raised, %d rejected."),
                  AcceptedCount,
                  AcceptedCount == 1 ? TEXT("") : TEXT("s"),
                  RejectedCount)
            : LastRejection.IsEmpty()
                  ? TEXT("[CAIRNBACK_REQUIRED] Select a Kharuun Cairnback.")
                  : LastRejection);
}

void AEchoesPlayerController::HoldSelectedUnits()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Hold position is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned defenders first."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        FString Feedback;
        if (Entity != nullptr && Bridge->IssueCommand(
                echoes::sim::CommandType::Hold,
                EntityId,
                0,
                Bridge->SimToWorld(Entity->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("HOLD POSITION: %d defender%s anchored%s"),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            *RejectionSuffix));
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[HOLD_REJECTED] No selected entity can defend a position.")
                : LastRejection);
    }
}

void AEchoesPlayerController::GuardAtCursor()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Guard is unavailable."));
        return;
    }
    if (SelectedEntityIds.IsEmpty())
    {
        SetStatusMessage(TEXT("[NO_SELECTION] Select one or more owned defenders first."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target the owned entity to guard with the pointer or center reticle."));
        return;
    }
    const AEchoesEntityView* TargetView =
        Cast<AEchoesEntityView>(HitResult.GetActor());
    const echoes::sim::Entity* Target =
        TargetView != nullptr
            ? Bridge->FindEntity(TargetView->GetEntityId())
            : nullptr;
    if (Target == nullptr ||
        Target->owner != UEchoesSimulationSubsystem::LocalPlayerId)
    {
        SetStatusMessage(TEXT("[GUARD_TARGET_INVALID] Point at a live owned entity."));
        return;
    }

    int32 AcceptedCount = 0;
    int32 RejectedCount = 0;
    FString LastRejection;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        FString Feedback;
        if (Bridge->IssueCommand(
                echoes::sim::CommandType::Guard,
                EntityId,
                Target->id,
                Bridge->SimToWorld(Target->position),
                FutureWellChoice,
                Feedback))
        {
            ++AcceptedCount;
        }
        else
        {
            ++RejectedCount;
            LastRejection = Feedback;
        }
    }
    if (AcceptedCount > 0)
    {
        const FString RejectionSuffix =
            RejectedCount > 0
                ? FString::Printf(TEXT(", %d rejected."), RejectedCount)
                : TEXT(".");
        SetStatusMessage(FString::Printf(
            TEXT("GUARD: %d defender%s assigned to entity %u%s"),
            AcceptedCount,
            AcceptedCount == 1 ? TEXT("") : TEXT("s"),
            Target->id,
            *RejectionSuffix));
        ShowAcceptedCommandMarker(
            Bridge->SimToWorld(Target->position),
            EEchoesCommandMarkerType::Guard,
            AcceptedCount);
    }
    else
    {
        SetStatusMessage(
            LastRejection.IsEmpty()
                ? TEXT("[GUARD_REJECTED] No selected entity accepted the guard order.")
                : LastRejection);
    }
}

void AEchoesPlayerController::QuickSaveScenario()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr)
    {
        Feedback = TEXT("[SAVE_SIM_NOT_READY] No active scenario can be saved.");
    }
    else
    {
        Bridge->QuickSaveScenario(Feedback);
    }
    SetStatusMessage(Feedback, 6.0f);
}

void AEchoesPlayerController::QuickLoadScenario()
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    FString Feedback;
    if (Bridge == nullptr)
    {
        Feedback = TEXT("[LOAD_SIM_NOT_READY] Start a scenario before loading.");
    }
    else if (Bridge->QuickLoadScenario(Feedback))
    {
        ClearSelection();
        bControlGroupAssignmentArmed = false;
    }
    SetStatusMessage(Feedback, 7.0f);
}

void AEchoesPlayerController::CycleHudScale()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] UI scale could not be changed."));
        return;
    }
    const float CurrentScale = Settings->GetHudScale();
    const float NewScale =
        CurrentScale < 0.99f ? 1.0f
        : CurrentScale < 1.14f ? 1.15f
        : CurrentScale < 1.34f ? 1.35f
                               : 0.85f;
    Settings->SetHudScale(NewScale);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: UI scale set to %d%%."),
        FMath::RoundToInt(NewScale * 100.0f)));
}

void AEchoesPlayerController::ToggleHighContrast()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] High contrast could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsHighContrastHudEnabled();
    Settings->SetHighContrastHudEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: high-contrast HUD %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::ToggleReducedMotion()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Reduced motion could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsReducedMotionEnabled();
    Settings->SetReducedMotionEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: reduced camera motion %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::ToggleReducedFlashing()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Reduced flashing could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsReducedFlashingEnabled();
    Settings->SetReducedFlashingEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("ACCESSIBILITY: reduced combat flashing %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::ToggleEdgePan()
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Edge pan could not be changed."));
        return;
    }
    const bool bEnabled = !Settings->IsEdgePanEnabled();
    Settings->SetEdgePanEnabled(bEnabled);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("CONTROLS: screen-edge camera pan %s."),
        bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void AEchoesPlayerController::AdjustCameraPanSpeed(float Delta)
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Camera pan speed could not be changed."));
        return;
    }
    Settings->SetCameraPanSpeedScale(Settings->GetCameraPanSpeedScale() + Delta);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("CONTROLS: camera pan speed set to %d%%."),
        FMath::RoundToInt(Settings->GetCameraPanSpeedScale() * 100.0f)));
}

void AEchoesPlayerController::AdjustCameraZoomSpeed(float Delta)
{
    UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    if (Settings == nullptr)
    {
        SetStatusMessage(TEXT("[SETTINGS_UNAVAILABLE] Camera zoom speed could not be changed."));
        return;
    }
    Settings->SetCameraZoomScale(Settings->GetCameraZoomScale() + Delta);
    Settings->SaveSettings();
    SetStatusMessage(FString::Printf(
        TEXT("CONTROLS: camera zoom step set to %d%%."),
        FMath::RoundToInt(Settings->GetCameraZoomScale() * 100.0f)));
}

void AEchoesPlayerController::DecreaseCameraPanSpeed()
{
    AdjustCameraPanSpeed(-0.25f);
}

void AEchoesPlayerController::IncreaseCameraPanSpeed()
{
    AdjustCameraPanSpeed(0.25f);
}

void AEchoesPlayerController::DecreaseCameraZoomSpeed()
{
    AdjustCameraZoomSpeed(-0.25f);
}

void AEchoesPlayerController::IncreaseCameraZoomSpeed()
{
    AdjustCameraZoomSpeed(0.25f);
}

void AEchoesPlayerController::BuildAtCursor(
    echoes::sim::EntityType BuildingType)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Construction is unavailable."));
        return;
    }
    uint32 WorkerId = 0;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        if (Entity != nullptr &&
            Entity->type == echoes::sim::EntityType::Worker)
        {
            WorkerId = EntityId;
            break;
        }
    }
    if (WorkerId == 0)
    {
        SetStatusMessage(
            TEXT("[BUILD_REQUIRES_WORKER] Select a worker, point at open ground, then press B, N, or M."));
        return;
    }
    FHitResult HitResult;
    if (!TraceCommandTarget(HitResult))
    {
        SetStatusMessage(TEXT("[NO_WORLD_HIT] Target open battlefield ground with the pointer or keyboard reticle."));
        return;
    }
    FString Feedback;
    if (Bridge->IssueBuildCommand(
            WorkerId,
            BuildingType,
            HitResult.Location,
            Feedback))
    {
        SetStatusMessage(
            BuildingType == echoes::sim::EntityType::Barracks
                ? TEXT("PRODUCTION STRUCTURE: construction order queued.")
                : BuildingType == echoes::sim::EntityType::UtilityStructure
                      ? TEXT("FACTION UTILITY: construction order queued.")
                      : TEXT("LOGISTICS STRUCTURE: construction order queued."));
        ShowAcceptedCommandMarker(
            HitResult.Location,
            EEchoesCommandMarkerType::Build,
            1);
    }
    else
    {
        SetStatusMessage(Feedback);
    }
}

void AEchoesPlayerController::ShowAcceptedCommandMarker(
    const FVector& WorldLocation,
    EEchoesCommandMarkerType MarkerType,
    int32 AcceptedCount)
{
    UWorld* World = GetWorld();
    if (World == nullptr || AcceptedCount <= 0)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.ObjectFlags |= RF_Transient;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AEchoesCommandMarkerView* Marker = World->SpawnActor<AEchoesCommandMarkerView>(
        WorldLocation + FVector(0.0f, 0.0f, 8.0f),
        FRotator::ZeroRotator,
        SpawnParameters);
    if (Marker == nullptr)
    {
        UE_LOG(
            LogEchoes,
            Warning,
            TEXT("[ECHOES_COMMAND_MARKER_FAILED] accepted=%d authorityChanged=false"),
            AcceptedCount);
        return;
    }

    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const bool bReducedMotion =
        Settings != nullptr && Settings->IsReducedMotionEnabled();
    const bool bReducedFlashing =
        Settings != nullptr && Settings->IsReducedFlashingEnabled();
    Marker->InitializeMarker(MarkerType, bReducedMotion, bReducedFlashing);

    const TCHAR* MarkerLabel = TEXT("move");
    switch (MarkerType)
    {
        case EEchoesCommandMarkerType::AttackMove:
            MarkerLabel = TEXT("attack_move");
            break;
        case EEchoesCommandMarkerType::Patrol:
            MarkerLabel = TEXT("patrol");
            break;
        case EEchoesCommandMarkerType::Guard:
            MarkerLabel = TEXT("guard");
            break;
        case EEchoesCommandMarkerType::Build:
            MarkerLabel = TEXT("build");
            break;
        case EEchoesCommandMarkerType::Move:
            break;
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_COMMAND_MARKER] type=%s accepted=%d formation=%s collision=false authoritative=false reducedMotion=%s reducedFlashing=%s finalArt=false"),
        MarkerLabel,
        AcceptedCount,
        *GetFormationLabel(),
        bReducedMotion ? TEXT("true") : TEXT("false"),
        bReducedFlashing ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::ProduceUnit(echoes::sim::EntityType UnitType)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    PruneSelection();
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Production is unavailable."));
        return;
    }
    int32 Accepted = 0;
    FString LastFeedback;
    for (const uint32 EntityId : SelectedEntityIds)
    {
        const echoes::sim::Entity* Entity = Bridge->FindEntity(EntityId);
        const bool bCompatible = Entity != nullptr &&
            ((UnitType == echoes::sim::EntityType::Worker &&
              Entity->type == echoes::sim::EntityType::CommandCore) ||
             ((UnitType == echoes::sim::EntityType::Soldier ||
               UnitType == echoes::sim::EntityType::HeavyUnit ||
               UnitType == echoes::sim::EntityType::ScoutUnit) &&
              Entity->type == echoes::sim::EntityType::Barracks));
        if (!bCompatible)
        {
            continue;
        }
        FString Feedback;
        if (Bridge->IssueProductionCommand(EntityId, UnitType, Feedback))
        {
            ++Accepted;
        }
        else
        {
            LastFeedback = Feedback;
        }
    }
    if (Accepted > 0)
    {
        SetStatusMessage(
            FString::Printf(
                TEXT("%s: %d production order%s queued."),
                UnitType == echoes::sim::EntityType::Worker
                    ? TEXT("WORKER")
                    : UnitType == echoes::sim::EntityType::HeavyUnit
                          ? TEXT("HEAVY UNIT")
                          : UnitType == echoes::sim::EntityType::ScoutUnit
                                ? TEXT("SCOUT UNIT")
                                : TEXT("LINE UNIT"),
                Accepted,
                Accepted == 1 ? TEXT("") : TEXT("s")));
    }
    else
    {
        SetStatusMessage(
            LastFeedback.IsEmpty()
                ? TEXT("[NO_COMPATIBLE_PRODUCER] Select a headquarters for Q or a production structure for E, semicolon, or apostrophe.")
                : LastFeedback);
    }
}

void AEchoesPlayerController::ToggleTechnologyPanel()
{
    if (bTitleScreenVisible || bMissionBriefingVisible ||
        bPauseMenuVisible || bMatchResultVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Technologies are unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }

    if (bTechnologyPanelVisible)
    {
        bTechnologyPanelVisible = false;
        Bridge->SetScenarioPaused(bTechnologyPanelWasScenarioPaused);
        SetIgnoreMoveInput(bTechnologyPanelWasScenarioPaused);
        SetIgnoreLookInput(bTechnologyPanelWasScenarioPaused);
        SetStatusMessage(TEXT("TECHNOLOGY ARCHIVE CLOSED."));
    }
    else
    {
        const echoes::sim::Simulation* Simulation = Bridge->GetSimulation();
        const echoes::sim::PlayerState* Player =
            Simulation != nullptr
                ? Simulation->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId)
                : nullptr;
        const echoes::sim::ResearchType FirstTechnology =
            Player != nullptr &&
                    Player->faction == echoes::sim::Faction::KharuunAssemblies
                ? echoes::sim::ResearchType::KharuunEchoCartography
                : echoes::sim::ResearchType::MeridianPrismaticTargeting;
        TechnologyPanelFocusedTier =
            Player != nullptr && Player->HasCompletedResearch(FirstTechnology)
                ? 1
                : 0;
        bTechnologyPanelWasScenarioPaused = Bridge->IsScenarioPaused();
        bTechnologyPanelVisible = true;
        bSelectionButtonDown = false;
        Bridge->SetScenarioPaused(true);
        SetIgnoreMoveInput(true);
        SetIgnoreLookInput(true);
        SetStatusMessage(
            TEXT("TECHNOLOGY ARCHIVE — Up/Down chooses a tier; Enter activates it; Shift+R chooses the next available project."),
            3600.0f);
    }
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_TECHNOLOGY_PANEL] visible=%s paused=%s focusedTier=%d pointerRows=true keyboardFocus=true keyboardConfirm=true"),
        bTechnologyPanelVisible ? TEXT("true") : TEXT("false"),
        Bridge->IsScenarioPaused() ? TEXT("true") : TEXT("false"),
        TechnologyPanelFocusedTier + 1);
}

void AEchoesPlayerController::FocusPreviousTechnologyTier()
{
    if (!bTechnologyPanelVisible)
    {
        NudgeKeyboardTarget(FVector2D(0.0f, -1.0f));
        return;
    }
    TechnologyPanelFocusedTier =
        FMath::Clamp(TechnologyPanelFocusedTier - 1, 0, 1);
    SetStatusMessage(
        FString::Printf(
            TEXT("TECHNOLOGY ARCHIVE — Tier %d focused; press Enter to activate."),
            TechnologyPanelFocusedTier + 1),
        3600.0f);
}

void AEchoesPlayerController::FocusNextTechnologyTier()
{
    if (!bTechnologyPanelVisible)
    {
        NudgeKeyboardTarget(FVector2D(0.0f, 1.0f));
        return;
    }
    TechnologyPanelFocusedTier =
        FMath::Clamp(TechnologyPanelFocusedTier + 1, 0, 1);
    SetStatusMessage(
        FString::Printf(
            TEXT("TECHNOLOGY ARCHIVE — Tier %d focused; press Enter to activate."),
            TechnologyPanelFocusedTier + 1),
        3600.0f);
}

bool AEchoesPlayerController::HandleTechnologyPanelPointer(
    const FVector2D& ScreenPosition)
{
    if (!bTechnologyPanelVisible)
    {
        return false;
    }
    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);
    if (ViewportX <= 0 || ViewportY <= 0)
    {
        SetStatusMessage(TEXT("[VIEWPORT_UNAVAILABLE] Technology selection could not resolve the screen."));
        return true;
    }
    const UEchoesGameUserSettings* Settings = UEchoesGameUserSettings::Get();
    const FEchoesTechnologyPanelLayout Layout =
        FEchoesTechnologyPanelLayout::Build(
            FVector2D(ViewportX, ViewportY),
            Settings != nullptr ? Settings->GetHudScale() : 1.0f);
    if (Layout.CloseButton.IsInsideOrOn(ScreenPosition))
    {
        ToggleTechnologyPanel();
        return true;
    }
    for (int32 TierIndex = 0; TierIndex < 2; ++TierIndex)
    {
        if (Layout.TechnologyRows[TierIndex].IsInsideOrOn(ScreenPosition))
        {
            TechnologyPanelFocusedTier = TierIndex;
            ResearchTechnologyByTier(TierIndex);
            return true;
        }
    }
    return true;
}

void AEchoesPlayerController::TogglePauseMenu()
{
    if (bTechnologyPanelVisible)
    {
        ToggleTechnologyPanel();
        return;
    }
    if (bTitleScreenVisible || bMissionBriefingVisible || bMatchResultVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    if (Bridge == nullptr || !Bridge->IsScenarioReady())
    {
        SetStatusMessage(TEXT("[SIM_NOT_READY] Pause is unavailable."));
        return;
    }
    if (Bridge->GetMatchOutcome() != echoes::sim::MatchOutcome::Ongoing)
    {
        SetStatusMessage(TEXT("[MATCH_FINISHED] Press R to restart."));
        return;
    }
    bPauseMenuVisible = !bPauseMenuVisible;
    Bridge->SetScenarioPaused(bPauseMenuVisible);
    SetIgnoreMoveInput(bPauseMenuVisible);
    SetIgnoreLookInput(bPauseMenuVisible);
    SetStatusMessage(
        bPauseMenuVisible
            ? TEXT("FIELD MENU — Enter, Escape, or P resumes; R restarts.")
            : TEXT("MATCH RESUMED."),
        bPauseMenuVisible ? 3600.0f : 3.0f);
    UE_LOG(
        LogEchoes,
        Display,
        TEXT("[ECHOES_PAUSE_MENU] visible=%s paused=%s"),
        bPauseMenuVisible ? TEXT("true") : TEXT("false"),
        Bridge->IsScenarioPaused() ? TEXT("true") : TEXT("false"));
}

void AEchoesPlayerController::RestartScenario()
{
    if (bTitleScreenVisible || bMissionBriefingVisible)
    {
        return;
    }
    UEchoesSimulationSubsystem* Bridge =
        GetWorld() != nullptr
            ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>()
            : nullptr;
    ClearSelection();
    ClearControlGroups();
    bControlGroupAssignmentArmed = false;
    if (Bridge != nullptr && Bridge->RestartPrototypeScenario())
    {
        bRuntimeStateKnown = true;
        bTitleScreenVisible = false;
        bPauseMenuVisible = false;
        bTechnologyPanelVisible = false;
        bMatchResultVisible = false;
        bCampaignResult = false;
        bCampaignSuccess = false;
        CampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
        RecordedCampaignConsequence = echoes::sim::FutureWellChoice::Dormant;
        CampaignCommitStatus = EEchoesCampaignCommitStatus::NotApplicable;
        PresentedMatchOutcome = echoes::sim::MatchOutcome::Ongoing;
        PresentedCampaignOperation = EEchoesOperationMode::Skirmish;
        SetIgnoreMoveInput(false);
        SetIgnoreLookInput(false);
        SetStatusMessage(
            Bridge->GetOperationMode() == EEchoesOperationMode::CampaignPrologue
                ? TEXT("MISSION RESTARTED — Mara Vey's archive recovery begins again from the deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignSevenAccounts
                ? TEXT("MISSION RESTARTED — Oruun's migration begins again from the inherited route state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignCityReserve
                ? TEXT("MISSION RESTARTED — Mara Vey's reserve grid returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignUnburiedRoad
                ? TEXT("MISSION RESTARTED — Oruun's road recovery returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignTermsOfContinuance
                ? TEXT("MISSION RESTARTED — the Meridian-authoritative treaty proxy scenario returns to its deterministic initial state.")
            : Bridge->GetOperationMode() ==
                    EEchoesOperationMode::CampaignNamesWithoutBirths
                ? TEXT("MISSION RESTARTED — Talar's protected census recovery returns to its deterministic initial state.")
                : TEXT("MATCH RESTARTED — deterministic initial state restored."));
        UE_LOG(LogEchoes, Display, TEXT("[ECHOES_RESULT_RESTARTED] outcome=0"));
    }
    else
    {
        NotifyRuntimeFailure(TEXT("ECHOES_MATCH_RESTART_FAILED"));
    }
}

void AEchoesPlayerController::SetFutureWellChoice(
    echoes::sim::FutureWellChoice Choice)
{
    if (IsModalOverlayVisible())
    {
        return;
    }
    FutureWellChoice = Choice;
    SetStatusMessage(
        FString::Printf(
            TEXT("Future Well protocol set to %s. Right-click a dormant Well with a worker selected."),
            *GetFutureWellChoiceLabel()),
        5.0f);
}

void AEchoesPlayerController::SetStatusMessage(
    const FString& Message,
    float DisplaySeconds)
{
    StatusMessage = Message;
    StatusMessageExpiresAt =
        GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() + DisplaySeconds : 0.0;
    UE_LOG(LogEchoes, Display, TEXT("[ECHOES_PLAYER_FEEDBACK] %s"), *Message);
}

bool AEchoesPlayerController::IsDraggingSelection() const
{
    return bSelectionButtonDown &&
           FVector2D::Distance(
               SelectionStartScreenPosition,
               SelectionCurrentScreenPosition) >= DragSelectionThresholdPixels;
}

FVector2D AEchoesPlayerController::GetSelectionStartScreenPosition() const
{
    return SelectionStartScreenPosition;
}

FVector2D AEchoesPlayerController::GetSelectionCurrentScreenPosition() const
{
    return SelectionCurrentScreenPosition;
}

const TArray<uint32>& AEchoesPlayerController::GetSelectedEntityIds() const
{
    return SelectedEntityIds;
}

echoes::sim::FutureWellChoice AEchoesPlayerController::GetFutureWellChoice() const
{
    return FutureWellChoice;
}

FString AEchoesPlayerController::GetFutureWellChoiceLabel() const
{
    switch (FutureWellChoice)
    {
        case echoes::sim::FutureWellChoice::Harvest:
            return TEXT("HARVEST");
        case echoes::sim::FutureWellChoice::Preserve:
            return TEXT("PRESERVE");
        case echoes::sim::FutureWellChoice::Reshape:
            return TEXT("RESHAPE");
        case echoes::sim::FutureWellChoice::Dormant:
            return TEXT("DORMANT");
    }
    return TEXT("UNKNOWN");
}

FString AEchoesPlayerController::GetStatusMessage() const
{
    if (GetWorld() == nullptr || GetWorld()->GetTimeSeconds() > StatusMessageExpiresAt)
    {
        return FString();
    }
    return StatusMessage;
}

FString AEchoesPlayerController::CommandLabel(
    echoes::sim::CommandType CommandType) const
{
    switch (CommandType)
    {
        case echoes::sim::CommandType::Stop:
            return TEXT("STOP");
        case echoes::sim::CommandType::Move:
            return TEXT("MOVE");
        case echoes::sim::CommandType::Gather:
            return TEXT("GATHER MATTER");
        case echoes::sim::CommandType::Deliver:
            return TEXT("DELIVER MATTER");
        case echoes::sim::CommandType::Build:
            return TEXT("BUILD");
        case echoes::sim::CommandType::Attack:
            return TEXT("ATTACK");
        case echoes::sim::CommandType::FutureWell:
            return FString::Printf(TEXT("FUTURE WELL: %s"), *GetFutureWellChoiceLabel());
        case echoes::sim::CommandType::Produce:
            return TEXT("PRODUCE");
        case echoes::sim::CommandType::AttackMove:
            return TEXT("ATTACK-MOVE");
        case echoes::sim::CommandType::Hold:
            return TEXT("HOLD POSITION");
        case echoes::sim::CommandType::Guard:
            return TEXT("GUARD");
        case echoes::sim::CommandType::Patrol:
            return TEXT("PATROL");
        case echoes::sim::CommandType::ToggleDeploy:
            return TEXT("TOGGLE BULWARK DEPLOYMENT");
        case echoes::sim::CommandType::ActivateRelaySupply:
            return TEXT("ACTIVATE RELAY SUPPLY");
        case echoes::sim::CommandType::ToggleWaystoneRoot:
            return TEXT("TOGGLE WAYSTONE ROOT");
        case echoes::sim::CommandType::AdaptWarform:
            return TEXT("ADAPT WARFORM");
        case echoes::sim::CommandType::RaiseMineralCover:
            return TEXT("RAISE MINERAL COVER");
        case echoes::sim::CommandType::Research:
            return TEXT("RESEARCH");
    }
    return TEXT("ORDER");
}
