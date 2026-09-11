// Author: Angelis Pseftis
#include "EchoesPlayerController.h"
#include "EchoesResourceMonitorModel.h"
#include "EchoesSimulationSubsystem.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "EchoesResourceMonitor"

namespace
{
/**
 * Simulation identifiers are opaque handles, not quantities. Locale grouping
 * would render entity 10247 as "10,247" and read as a resource amount.
 */
FText EntityLabel(const uint64 Identifier)
{
    return FText::AsNumber(Identifier, &FNumberFormattingOptions::DefaultNoGrouping());
}
}

bool AEchoesPlayerController::OpenResourceMonitor()
{
    if (IsReplayInputActive()) return false;
    if (PlayerFlow.Current() == EEchoesShellScreen::Gameplay)
    {
        if (!bOnlineLocalMenuVisible)
        {
            if (IsModalOverlayVisible()) return false;
            TogglePauseMenu();
        }
    }
    const bool bLocalPause = PlayerFlow.Current() == EEchoesShellScreen::Pause;
    const bool bOnlineMenu = PlayerFlow.Current() == EEchoesShellScreen::Gameplay &&
        bOnlineLocalMenuVisible && IsActiveOnlineNetworkMatch();
    if (!bLocalPause && !bOnlineMenu) return false;
    PlayerFlow.Push(EEchoesShellScreen::ResourceMonitor);
    RefreshShell();
    RefreshFieldHud();
    return true;
}

void AEchoesPlayerController::BuildResourceMonitorShellView(FEchoesShellView& View) const
{
    View.Title = LOCTEXT("Title", "Resource monitor");
    View.Eyebrow = LOCTEXT("Eyebrow", "ECONOMY AND COMMITMENTS");
    View.Buttons.Reset();
    View.Buttons.Add({LOCTEXT("Back", "Back"), EEchoesShellAction::Back});
    TOptional<FEchoesResourceMonitorView> Snapshot;
    if (!IsReplayInputActive() && GetNetMode() == NM_Client)
    {
        if (const auto* Scoped = GetNetworkScopedView())
            Snapshot = FEchoesResourceMonitorModel::BuildNetworkScoped(*Scoped);
    }
    else if (!IsReplayInputActive())
    {
        const auto* Bridge = GetWorld() ? GetWorld()->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
        const auto* Simulation = Bridge ? Bridge->GetSimulation() : nullptr;
        if (Simulation)
            if (const auto Scoped = Simulation->CreatePlayerView(UEchoesSimulationSubsystem::LocalPlayerId))
                Snapshot = FEchoesResourceMonitorModel::Build(*Scoped);
    }
    if (!Snapshot.IsSet())
    {
        View.Body = LOCTEXT("Unavailable", "Economy information is unavailable in this view.");
        return;
    }
    const auto& Data = Snapshot.GetValue();
    View.Status = IsOnlineLocalMenuShellRouteActive()
        ? LOCTEXT("Online", "ONLINE MATCH CONTINUES — local controls are held while this menu is open.")
        : LOCTEXT("Paused", "Match paused. Values reflect the current match state.");
    TArray<FText> Lines;
    Lines.Add(FText::Format(LOCTEXT("Liquid", "AVAILABLE NOW\nMatter {0}  ·  Dawn {1}\nLogistics {2} / {3}"),
        FText::AsNumber(Data.LiquidFunds.Matter), FText::AsNumber(Data.LiquidFunds.Dawn),
        FText::AsNumber(Data.Logistics.Used), FText::AsNumber(Data.Logistics.Capacity)));
    if (Data.Logistics.bMobileEntityCountAvailable)
        Lines.Add(FText::Format(LOCTEXT("ArmyLimit", "Army {0} fielded + {1} in production of {2} controllable units. Fielded plus in-production is what new production is checked against; Logistics is a separate limit."),
            FText::AsNumber(Data.Logistics.MobileEntitiesFielded), FText::AsNumber(Data.Logistics.MobileEntitiesInProduction),
            FText::AsNumber(Data.Logistics.MobileEntityLimit)));
    else Lines.Add(LOCTEXT("ArmyUnavailable", "Army count against the controllable-unit limit: unavailable in this view."));
    if (Data.Logistics.bTemporaryCapacityAvailable)
        Lines.Add(FText::Format(LOCTEXT("TemporaryCapacity", "Connected Relay capacity: {0} temporary Logistics. Permanent-capacity breakdown: unavailable."),
            FText::AsNumber(Data.Logistics.TemporaryCapacityFromActiveRelays)));
    Lines.Add(LOCTEXT("IncomeUnavailable", "REALIZED INCOME\n30-second and 60-second credited income: unavailable."));
    const auto& Workers = Data.Workers;
    Lines.Add(FText::Format(LOCTEXT("WorkerTotal", "WORKERS\nOwned workers: {0}"), FText::AsNumber(Workers.Total)));
    if (Workers.bActivityAvailable)
        Lines.Add(FText::Format(LOCTEXT("WorkerActivity", "Idle {0}  ·  Gathering {1}  ·  Delivering {2}\nTraveling resource routes {3}  ·  Building {4}  ·  Repairing {5}  ·  Other assignments {6}"),
            FText::AsNumber(Workers.Idle), FText::AsNumber(Workers.Gathering), FText::AsNumber(Workers.Delivering),
            FText::AsNumber(Workers.TravelingResourceRoute), FText::AsNumber(Workers.Building),
            FText::AsNumber(Workers.Repairing), FText::AsNumber(Workers.OtherAssigned)));
    else Lines.Add(LOCTEXT("WorkerActivityUnavailable", "Worker activity details: unavailable."));
    Lines.Add(LOCTEXT("RouteUnavailable", "Blocked-worker count and measured route time: unavailable."));
    if (Data.Availability.bWorkerSourceAssignmentsAvailable)
    {
        Lines.Add(LOCTEXT("Assignments", "MATTER ASSIGNMENTS"));
        if (Data.MatterSourceAssignments.IsEmpty()) Lines.Add(LOCTEXT("NoAssignments", "No workers assigned to a Matter source."));
        for (const auto& Row : Data.MatterSourceAssignments)
            Lines.Add(FText::Format(LOCTEXT("Assignment", "Source #{0}: {1} assigned workers"), EntityLabel(Row.ResourceEntityId), FText::AsNumber(Row.AssignedWorkers)));
    }
    Lines.Add(LOCTEXT("DepositUnavailable", "Known deposit amounts, saturation and depletion estimates: unavailable."));
    Lines.Add(LOCTEXT("Commitments", "COMMITMENTS"));
    if (Data.bNetworkScoped)
        Lines.Add(LOCTEXT("CommitmentsUnavailable", "Detailed invested costs and waiting requests: unavailable."));
    else
    {
        if (Data.ActiveInvestedCommitments.IsEmpty()) Lines.Add(LOCTEXT("NoActive", "No active construction or production investments."));
        for (const auto& Row : Data.ActiveInvestedCommitments)
            Lines.Add(FText::Format(LOCTEXT("Invested", "{0} #{1}: already invested {2} Matter / {3} Dawn; {4}% complete."),
                Row.bConstruction ? LOCTEXT("Construction", "Construction") : LOCTEXT("Producer", "Producer"),
                EntityLabel(Row.OwnerEntityId), FText::AsNumber(Row.InvestedMatter), FText::AsNumber(Row.InvestedDawn),
                FText::AsNumber(FMath::Clamp(int64(Row.Progress) * 100 / FMath::Max(1, Row.RequiredProgress), int64(0), int64(100)))));
        for (const auto& Row : Data.WaitingUninvestedRequests)
            Lines.Add(FText::Format(LOCTEXT("Waiting", "Producer #{0}, waiting item #{1}: requests {2} Matter / {3} Dawn / {4} Logistics. Not yet invested or reserved."),
                EntityLabel(Row.ProducerEntityId), EntityLabel(Row.ProductionItemId), FText::AsNumber(Row.RequestedMatter),
                FText::AsNumber(Row.RequestedDawn), FText::AsNumber(Row.RequestedLogistics)));
        Lines.Add(FText::Format(LOCTEXT("Reserved", "Active production reserves {0} Logistics. Paid costs above are already deducted from available funds."), FText::AsNumber(Data.Logistics.ActiveReserved)));
    }
    Lines.Add(LOCTEXT("ResearchUnavailable", "Detailed research investment and the next affordability failure: unavailable."));
    const auto AddTimers = [&Lines](const FText& Heading, bool bAvailable, const TArray<FEchoesResourceMonitorTimer>& Timers)
    {
        Lines.Add(Heading);
        if (!bAvailable) { Lines.Add(LOCTEXT("TimerUnavailable", "Timing details unavailable.")); return; }
        if (Timers.IsEmpty()) { Lines.Add(LOCTEXT("NoTimers", "None active.")); return; }
        for (const auto& Timer : Timers)
            Lines.Add(FText::Format(LOCTEXT("Timer", "Source #{0}: {1} seconds remaining"), EntityLabel(Timer.SourceEntityId),
                FText::AsNumber((Timer.RemainingTicks + 19) / 20)));
    };
    AddTimers(LOCTEXT("Relay", "RELAY EXPIRY"), Data.Availability.bRelayExpiryAvailable, Data.RelayExpiries);
    AddTimers(LOCTEXT("Preserve", "NEXT PRESERVE OPPORTUNITIES"), Data.Availability.bPreserveReturnAvailable, Data.PreserveReturns);
    Lines.Add(LOCTEXT("PreserveCondition", "Preserve pays only while the Well remains uncontested under your control."));
    AddTimers(LOCTEXT("Coherence", "CHOIR COHERENCE CHARGES"), Data.Availability.bChoirCoherenceTimersAvailable, Data.ChoirCoherenceCharges);
    Lines.Add(LOCTEXT("Rounding", "Timers round up to the next whole second. Information is limited to your scoped match view."));
    View.Body = FText::Join(FText::FromString(TEXT("\n\n")), Lines);
}
#undef LOCTEXT_NAMESPACE
