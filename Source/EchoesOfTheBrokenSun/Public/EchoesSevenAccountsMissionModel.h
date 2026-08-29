#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesSevenAccountsPhase : uint8
{
    Inactive,
    EstablishWaystone,
    RecallMemory,
    Complete,
    Failed
};

struct FEchoesSevenAccountsMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bMemoryBearerIntact = false;
    bool bWaystoneIntact = false;
    bool bWaystoneRootedAtAnchor = false;
    bool bMemoryBearerAtAccountSite = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesSevenAccountsRoute final
{
    echoes::sim::FutureWellChoice PriorChoice =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 WaystoneAnchor{};
    echoes::sim::Vec2 MemoryAccountSite{};
    const TCHAR* StableName = TEXT("unavailable");
    const TCHAR* DisplayName = TEXT("UNAVAILABLE ROUTE");
};

/** Pure mission-02 reducer and authored consequence-to-route mapping. */
struct ECHOESOFTHEBROKENSUN_API FEchoesSevenAccountsMissionModel final
{
    [[nodiscard]] static EEchoesSevenAccountsPhase DeterminePhase(
        const FEchoesSevenAccountsMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesSevenAccountsPhase Phase);
    [[nodiscard]] static FEchoesSevenAccountsRoute RouteForChoice(
        echoes::sim::FutureWellChoice Choice);
};
