#pragma once

#include "CoreMinimal.h"
#include "EchoesFutureThatWonMissionModel.h"
#include "EchoesSimCore/Simulation.h"

enum class EEchoesAssemblyOfTheMissingPhase : uint8
{
    Inactive,
    EstablishPublicRecordReadback,
    LinkCrownfallIndex,
    ObserveAssembly,
    Complete,
    Failed
};

struct FEchoesAssemblyOfTheMissingMissionFacts final
{
    bool bOperationActive = false;
    bool bLocalCoreIntact = false;
    bool bOruunIntact = false;
    bool bVerifierIntact = false;
    bool bPublicInterfacesIntact = false;
    bool bPublicRecordReadbackEstablished = false;
    bool bCrownfallIndexLinked = false;
    bool bMeridianAssemblyWitnessObserved = false;
    bool bKharuunAssemblyWitnessObserved = false;
    bool bSkirmishStillOngoing = true;
};

struct FEchoesAssemblyOfTheMissingPlan final
{
    echoes::sim::FutureWellChoice FoundingDoctrine =
        echoes::sim::FutureWellChoice::Dormant;
    echoes::sim::FutureWellChoice RecordedProtocol =
        echoes::sim::FutureWellChoice::Dormant;
    EEchoesCityDistrict FirstContributingDistrict =
        EEchoesCityDistrict::LifeSupport;
    EEchoesCityDistrict SecondContributingDistrict =
        EEchoesCityDistrict::Transit;
    EEchoesCityDistrict DeferredDistrict =
        EEchoesCityDistrict::Archive;
    echoes::sim::Vec2 MeridianPublicRecordSite;
    echoes::sim::Vec2 KharuunPublicRecordSite;
    echoes::sim::Vec2 CrownfallIndexSite;
    echoes::sim::Vec2 MeridianAssemblyWitnessSite;
    echoes::sim::Vec2 KharuunAssemblyWitnessSite;
    uint8 StablePlanKey = 0;
    const TCHAR* RouteStableName = TEXT("unavailable");
    const TCHAR* RouteDisplayName = TEXT("UNAVAILABLE ROUTE");
    const TCHAR* ProtocolStableName = TEXT("unavailable");
    const TCHAR* ProtocolDisplayName = TEXT("UNAVAILABLE PROTOCOL");
};

/**
 * Pure Mission 13 reducer and exact twelve-record public assembly projection.
 * It records bounded public observations only; it does not assign authorship,
 * responsibility, consent, trust, restoration, or mixed-faction authority.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesAssemblyOfTheMissingMissionModel final
{
    [[nodiscard]] static EEchoesAssemblyOfTheMissingPhase DeterminePhase(
        const FEchoesAssemblyOfTheMissingMissionFacts& Facts);
    [[nodiscard]] static const TCHAR* StableName(
        EEchoesAssemblyOfTheMissingPhase Phase);
    [[nodiscard]] static bool TryPlanForLedger(
        echoes::sim::FutureWellChoice FoundingDoctrine,
        uint8 ReserveAuthorityFacts,
        echoes::sim::FutureWellChoice RecordedProtocol,
        FEchoesAssemblyOfTheMissingPlan& OutPlan);
};
