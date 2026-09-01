#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesAssemblyOfTheMissingMissionModel.h"
#include "EchoesBrokenSunMissionModel.h"
#include "EchoesCampaignProgress.h"
#include "EchoesFutureThatWonMissionModel.h"
#include "EchoesNoNeutralLedgerMissionModel.h"
#include "EchoesSeveralVoicesOneCommandMissionModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCampaignPlanMatrixTest,
    "Echoes.Runtime.Campaign.PlanMatrix",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

namespace
{
using echoes::sim::FutureWellChoice;

const TCHAR* PlanMatrixChoiceName(const FutureWellChoice Choice)
{
    switch (Choice)
    {
        case FutureWellChoice::Dormant:
            return TEXT("dormant");
        case FutureWellChoice::Harvest:
            return TEXT("harvest");
        case FutureWellChoice::Preserve:
            return TEXT("preserve");
        case FutureWellChoice::Reshape:
            return TEXT("reshape");
    }
    return TEXT("unknown");
}

const TCHAR* PlanMatrixDistrictName(const EEchoesCityDistrict District)
{
    switch (District)
    {
        case EEchoesCityDistrict::LifeSupport:
            return TEXT("life-support");
        case EEchoesCityDistrict::Transit:
            return TEXT("transit");
        case EEchoesCityDistrict::Archive:
            return TEXT("archive");
    }
    return TEXT("unknown");
}

/** Runs all five chained downstream planners for one ledger tuple and
 *  reports whether they agree, alongside the Mission 15 plan. */
struct FPlanMatrixVerdict final
{
    bool bAgreed = false;
    bool bPlanned = false;
    FEchoesNoNeutralLedgerPlan LedgerPlan;
    FEchoesBrokenSunPlan FinalPlan;
};

FPlanMatrixVerdict RunAllPlanners(
    const FutureWellChoice FoundingDoctrine,
    const uint8 ReserveFacts,
    const FutureWellChoice RecordedProtocol)
{
    FPlanMatrixVerdict Verdict;
    FEchoesFutureThatWonPlan ActivationPlan;
    FEchoesAssemblyOfTheMissingPlan AssemblyPlan;
    FEchoesSeveralVoicesOneCommandPlan VoicesPlan;
    const bool bLedger = FEchoesNoNeutralLedgerMissionModel::TryPlanForLedger(
        FoundingDoctrine, ReserveFacts, RecordedProtocol, Verdict.LedgerPlan);
    const bool bActivation =
        FEchoesFutureThatWonMissionModel::TryPlanForLedger(
            FoundingDoctrine, ReserveFacts, RecordedProtocol, ActivationPlan);
    const bool bAssembly =
        FEchoesAssemblyOfTheMissingMissionModel::TryPlanForLedger(
            FoundingDoctrine, ReserveFacts, RecordedProtocol, AssemblyPlan);
    const bool bVoices =
        FEchoesSeveralVoicesOneCommandMissionModel::TryPlanForLedger(
            FoundingDoctrine, ReserveFacts, RecordedProtocol, VoicesPlan);
    const bool bFinal = FEchoesBrokenSunMissionModel::TryPlanForLedger(
        FoundingDoctrine, ReserveFacts, RecordedProtocol, Verdict.FinalPlan);
    Verdict.bPlanned = bLedger;
    Verdict.bAgreed = bLedger == bActivation && bLedger == bAssembly &&
                      bLedger == bVoices && bLedger == bFinal;
    return Verdict;
}
} // namespace

bool FEchoesCampaignPlanMatrixTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FutureWellChoice AllChoices[] = {
        FutureWellChoice::Dormant,
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        FutureWellChoice::Reshape};
    const uint8 LifeBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::LifeSupportPowered);
    const uint8 TransitBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::TransitPowered);
    const uint8 ArchiveBit = static_cast<uint8>(
        EEchoesReserveAuthorityCompletionFact::ArchivePowered);
    const uint8 DistrictBits = LifeBit | TransitBit | ArchiveBit;

    // Full-boundary sweep: every founding doctrine, every recorded protocol,
    // and every district-bit subset — each once bare and once inside the
    // ledger's ordinary non-district completion-fact context.
    int32 ReachableCount = 0;
    for (const FutureWellChoice FoundingDoctrine : AllChoices)
    {
        for (const FutureWellChoice RecordedProtocol : AllChoices)
        {
            for (uint8 Subset = 0; Subset <= DistrictBits; ++Subset)
            {
                if ((Subset & ~DistrictBits) != 0)
                {
                    continue;
                }
                for (const uint8 ContextBits :
                     {static_cast<uint8>(0), static_cast<uint8>(0x78)})
                {
                    const uint8 ReserveFacts = Subset | ContextBits;
                    const bool bExpectReachable =
                        FoundingDoctrine != FutureWellChoice::Dormant &&
                        RecordedProtocol != FutureWellChoice::Dormant &&
                        FMath::CountBits(Subset) == 2;
                    const FPlanMatrixVerdict Verdict = RunAllPlanners(
                        FoundingDoctrine, ReserveFacts, RecordedProtocol);
                    TestTrue(
                        TEXT("All five downstream planners agree on one verdict"),
                        Verdict.bAgreed);
                    TestEqual(
                        TEXT("Planner reachability matches the authored boundary"),
                        Verdict.bPlanned,
                        bExpectReachable);
                    if (bExpectReachable)
                    {
                        ++ReachableCount;
                        TestTrue(
                            TEXT("No reachable plan dead-ends: an ending is always available"),
                            Verdict.FinalPlan.AvailableFinalResolutions != 0);
                        TestTrue(
                            TEXT("Controlled Stabilization is available on every reachable plan"),
                            (Verdict.FinalPlan.AvailableFinalResolutions &
                             static_cast<uint8>(
                                 EEchoesFinalResolutionAvailability::
                                     ControlledStabilization)) != 0);
                        TestEqual(
                            TEXT("The final plan carries the ledger plan's stable key"),
                            Verdict.FinalPlan.StablePlanKey,
                            Verdict.LedgerPlan.StablePlanKey);
                        const EEchoesCityDistrict First =
                            Verdict.LedgerPlan.FirstContributingDistrict;
                        const EEchoesCityDistrict Second =
                            Verdict.LedgerPlan.SecondContributingDistrict;
                        const EEchoesCityDistrict Deferred =
                            Verdict.LedgerPlan.DeferredDistrict;
                        TestTrue(
                            TEXT("Contributing and deferred districts partition the city"),
                            First != Second && First != Deferred &&
                                Second != Deferred);
                        const uint8 DeferredBit =
                            Deferred == EEchoesCityDistrict::LifeSupport
                                ? LifeBit
                            : Deferred == EEchoesCityDistrict::Transit
                                ? TransitBit
                                : ArchiveBit;
                        TestTrue(
                            TEXT("The deferred district is exactly the unpowered one"),
                            (ReserveFacts & DeferredBit) == 0);
                        TestTrue(
                            TEXT("The final convergence never aliases the inherited approach"),
                            !(Verdict.FinalPlan.FinalConvergenceSite ==
                              Verdict.FinalPlan.CrownfallApproachSite));
                    }
                    else
                    {
                        TestTrue(
                            TEXT("An unreachable tuple fails closed to the zeroed plan"),
                            Verdict.FinalPlan.AvailableFinalResolutions == 0 &&
                                FCString::Strcmp(
                                    Verdict.LedgerPlan.RouteStableName,
                                    TEXT("unavailable")) == 0);
                    }
                }
            }
        }
    }
    TestEqual(
        TEXT("The reachable space is exactly 27 plans in each fact context"),
        ReachableCount,
        54);

    // Evidence matrix: the 27 canonical reachable plans against their earned
    // ending outcomes, in the ledger's ordinary completion-fact context.
    const FutureWellChoice PlayerChoices[] = {
        FutureWellChoice::Harvest,
        FutureWellChoice::Preserve,
        FutureWellChoice::Reshape};
    TSet<uint8> MatrixKeys;
    uint8 CoveredEndings = 0;
    for (const FutureWellChoice FoundingDoctrine : PlayerChoices)
    {
        for (const uint8 DeferredBit : {LifeBit, TransitBit, ArchiveBit})
        {
            for (const FutureWellChoice RecordedProtocol : PlayerChoices)
            {
                const uint8 ReserveFacts = static_cast<uint8>(
                    0x78 | (DistrictBits & ~DeferredBit));
                FEchoesBrokenSunPlan Plan;
                if (!TestTrue(
                        TEXT("A canonical reachable tuple plans Mission 15"),
                        FEchoesBrokenSunMissionModel::TryPlanForLedger(
                            FoundingDoctrine,
                            ReserveFacts,
                            RecordedProtocol,
                            Plan)))
                {
                    continue;
                }
                MatrixKeys.Add(Plan.StablePlanKey);
                CoveredEndings |= Plan.AvailableFinalResolutions;
                FString Endings = TEXT("controlled-stabilization");
                if ((Plan.AvailableFinalResolutions &
                     static_cast<uint8>(
                         EEchoesFinalResolutionAvailability::Restoration)) != 0)
                {
                    Endings += TEXT(",restoration");
                }
                if ((Plan.AvailableFinalResolutions &
                     static_cast<uint8>(EEchoesFinalResolutionAvailability::
                                            Extinguishment)) != 0)
                {
                    Endings += TEXT(",extinguishment");
                }
                if ((Plan.AvailableFinalResolutions &
                     static_cast<uint8>(EEchoesFinalResolutionAvailability::
                                            OpenEvolution)) != 0)
                {
                    Endings += TEXT(",open-evolution");
                }
                AddInfo(FString::Printf(
                    TEXT("plan-matrix key=%02d doctrine=%s protocol=%s ")
                    TEXT("deferred=%s route=%s hold=%llu endings=%s"),
                    Plan.StablePlanKey,
                    PlanMatrixChoiceName(FoundingDoctrine),
                    PlanMatrixChoiceName(RecordedProtocol),
                    PlanMatrixDistrictName(Plan.DeferredDistrict),
                    Plan.RouteStableName,
                    Plan.ResolutionHoldTicks,
                    *Endings));
            }
        }
    }
    TestEqual(TEXT("The evidence matrix holds 27 unique plan keys"),
              MatrixKeys.Num(), 27);
    TestEqual(TEXT("The 27 plans jointly reach all four named endings"),
              CoveredEndings, static_cast<uint8>(0x0F));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
