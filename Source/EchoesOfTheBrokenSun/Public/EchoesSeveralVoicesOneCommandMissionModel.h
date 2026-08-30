#pragma once

#include "CoreMinimal.h"
#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesSeveralVoicesOneCommandPhase : uint8
{
    Inactive,
    ResearchHeldAlternatives,
    ResolveIncompatibleVoices,
    ResearchSharedResolution,
    AnchorCrisis,
    HoldSharedResolution,
    Complete,
    Failed
};

struct FEchoesSeveralVoicesOneCommandMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bPossibleVoiceIntact = false;
    bool bManifestVoiceIntact = false;
    bool bNemeIntact = false;
    bool bResearchLoomIntact = false;
    bool bHeldAlternativesResearched = false;
    bool bPossibleVoiceResolved = false;
    bool bPossibleVoiceAtSite = false;
    bool bManifestVoiceResolved = false;
    bool bManifestVoiceAtSite = false;
    bool bNemeAtCommandSite = false;
    bool bSharedResolutionResearched = false;
    bool bPhaseAnchorComplete = false;
    bool bCrisisWindowHeld = false;
    bool bCrisisContractFailed = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesSeveralVoicesOneCommandPlan final
{
    echoes::sim::FutureWellChoice FoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice RecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::Vec2 PossibleVoiceSite;
    echoes::sim::Vec2 ManifestVoiceSite;
    echoes::sim::Vec2 NemeCommandSite;
    echoes::sim::Vec2 CrisisAnchorSite;
    uint8 StablePlanKey = 0;
    const TCHAR* RouteStableName = TEXT("unavailable");
    const TCHAR* RouteDisplayName = TEXT("UNAVAILABLE ROUTE");
    const TCHAR* ProtocolStableName = TEXT("unavailable");
    const TCHAR* ProtocolDisplayName = TEXT("UNAVAILABLE PROTOCOL");
};

/**
 * Pure Mission 14 reducer. The mission grants command only over a Hollow Choir
 * force and requires two incompatible, resolved identities to remain at
 * separate inherited sites while Neme anchors one bounded crisis window.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesSeveralVoicesOneCommandMissionModel final
{
    [[nodiscard]] static EEchoesSeveralVoicesOneCommandPhase DeterminePhase(
        const FEchoesSeveralVoicesOneCommandMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesSeveralVoicesOneCommandPhase Phase);
    [[nodiscard]] static bool TryPlanForLedger(
        echoes::sim::FutureWellChoice FoundingDoctrine,
        uint8 ReserveAuthorityFacts,
        echoes::sim::FutureWellChoice RecordedProtocol,
        FEchoesSeveralVoicesOneCommandPlan& OutPlan);
};
