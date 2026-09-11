// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "EchoesProductionReasonText.h"
#include "EchoesSimCore/Simulation.h"
#include "EchoesSimulationSubsystem.h"
#include "EchoesSkirmishSetup.h"
#include "EchoesTestSaveEnvironment.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesProductionRefusalTest,
    "Echoes.Runtime.Gameplay.ProductionRefusalText",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

/**
 * Owner play test 2026-09-11: with 1,580 Matter and 0 Dawn every production
 * order answered "[INSUFFICIENT_RESOURCES] The selected unit cannot be
 * funded." The refusal must name the unit, its price, the holding, the short
 * resource and where that resource comes from, in the status line and on the
 * deck alike.
 */
bool FEchoesProductionRefusalTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    using namespace echoes::sim;

    const FutureWellRules Well = DefaultSimulationRules().futureWell;
    const FString DawnShort = FEchoesProductionReasonText::FundingRefusal(
        ProductionStartBlockReason::InsufficientDawn, TEXT("LANCER"),
        ResourcePool{85, 20}, ResourcePool{1580, 0}, Well, 20);
    TestTrue(TEXT("Dawn shortfall carries its tag"), DawnShort.StartsWith(TEXT("[INSUFFICIENT_DAWN]")));
    TestTrue(TEXT("Dawn shortfall names the unit"), DawnShort.Contains(TEXT("LANCER")));
    TestTrue(TEXT("Dawn shortfall states the price"), DawnShort.Contains(TEXT("85 Matter / 20 Dawn")));
    TestTrue(TEXT("Dawn shortfall states the holding with separators"),
        DawnShort.Contains(TEXT("1,580 Matter / 0 Dawn")));
    TestTrue(TEXT("Dawn shortfall names the source of Dawn"), DawnShort.Contains(TEXT("Future Well")));
    TestTrue(TEXT("Dawn shortfall quotes the configured Harvest yield"),
        DawnShort.Contains(FText::AsNumber(Well.harvestImmediateDawn).ToString()));
    TestTrue(TEXT("Dawn shortfall quotes the configured Preserve cadence in seconds"),
        DawnShort.Contains(FString::Printf(TEXT("%s every %llu s"),
            *FText::AsNumber(Well.preserveDawnPerInterval).ToString(),
            static_cast<unsigned long long>((Well.preserveIntervalTicks + 19) / 20))));

    const FString MatterShort = FEchoesProductionReasonText::FundingRefusal(
        ProductionStartBlockReason::InsufficientMatter, TEXT("BULWARK TEAM"),
        ResourcePool{130, 25}, ResourcePool{40, 60}, Well, 20);
    TestTrue(TEXT("Matter shortfall carries its tag"), MatterShort.StartsWith(TEXT("[INSUFFICIENT_MATTER]")));
    TestTrue(TEXT("Matter shortfall names the source of Matter"),
        MatterShort.Contains(TEXT("Surveyors")) && MatterShort.Contains(TEXT("deposits")));
    TestTrue(TEXT("An unspecific reason is judged from the balances, Matter first"),
        FEchoesProductionReasonText::FundingRefusal(ProductionStartBlockReason::None, TEXT("X"),
            ResourcePool{100, 10}, ResourcePool{50, 50}, Well, 20).StartsWith(TEXT("[INSUFFICIENT_MATTER]")) &&
        FEchoesProductionReasonText::FundingRefusal(ProductionStartBlockReason::None, TEXT("X"),
            ResourcePool{100, 10}, ResourcePool{500, 5}, Well, 20).StartsWith(TEXT("[INSUFFICIENT_DAWN]")));

    TestEqual(TEXT("Tile price shows both resources"),
        FEchoesProductionReasonText::TileCost(ResourcePool{85, 20}).ToString(), FString(TEXT("85M 20D")));
    TestEqual(TEXT("Tile price omits zero Dawn"),
        FEchoesProductionReasonText::TileCost(ResourcePool{50, 0}).ToString(), FString(TEXT("50M")));
    TestTrue(TEXT("A busy producer with queue room has no availability warning"),
        FEchoesProductionReasonText::Availability(ProductionStartBlockReason::Busy).IsEmpty() &&
        FEchoesProductionReasonText::Availability(ProductionStartBlockReason::None).IsEmpty());
    TestTrue(TEXT("Dawn availability points at the Future Well"),
        FEchoesProductionReasonText::Availability(ProductionStartBlockReason::InsufficientDawn)
            .ToString().Contains(TEXT("Future Well")));
    TestTrue(TEXT("Logistics availability points at the Power Link"),
        FEchoesProductionReasonText::Availability(ProductionStartBlockReason::LogisticsCapacity)
            .ToString().Contains(TEXT("Power Link")));
    TestTrue(TEXT("Only missing or unfinished producers make a tile inert"),
        FEchoesProductionReasonText::IsStructural(ProductionStartBlockReason::ProducerIncomplete) &&
        FEchoesProductionReasonText::IsStructural(ProductionStartBlockReason::InvalidProducer) &&
        !FEchoesProductionReasonText::IsStructural(ProductionStartBlockReason::InsufficientMatter) &&
        !FEchoesProductionReasonText::IsStructural(ProductionStartBlockReason::MobileEntityLimit));
    TestEqual(TEXT("Meridian roster names resolve without a catalog"),
        FEchoesProductionReasonText::UnitName(Faction::MeridianCompact, EntityType::HeavyUnit, nullptr),
        FString(TEXT("BULWARK TEAM")));
    TestTrue(TEXT("Another faction without a catalog keeps the caller's label"),
        FEchoesProductionReasonText::UnitName(Faction::KharuunAssemblies, EntityType::Soldier, nullptr).IsEmpty());

    // The bridge path: a Scarce seat orders a Bulwark Team it cannot fund.
    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }
    FTestWorldWrapper WorldWrapper;
    if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
    {
        WorldWrapper.ForwardErrorMessages(this);
        AddError(TEXT("Could not create the production-refusal test world."));
        return false;
    }
    UWorld* World = WorldWrapper.GetTestWorld();
    UEchoesSimulationSubsystem* Bridge =
        World != nullptr ? World->GetSubsystem<UEchoesSimulationSubsystem>() : nullptr;
    if (!TestNotNull(TEXT("Refusal world owns the simulation subsystem"), Bridge) ||
        !TestTrue(TEXT("Refusal scenario starts"),
                  Bridge != nullptr && Bridge->StartPrototypeScenario()))
    {
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    FString Feedback;
    FEchoesSkirmishSetup ScarceSetup = Bridge->GetActiveSkirmishSetup();
    ScarceSetup.ResourceLevel = EEchoesSkirmishResourceLevel::Scarce;
    if (!TestTrue(TEXT("The Scarce deployment applies"), Bridge->ApplySkirmishSetup(ScarceSetup, Feedback)))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    const Simulation* Scenario = Bridge->GetSimulation();
    if (!TestNotNull(TEXT("Scarce scenario has deterministic state"), Scenario))
    {
        Bridge->StopPrototypeScenario();
        WorldWrapper.ForwardErrorMessages(this);
        return false;
    }
    uint32 Foundry = 0;
    Faction LocalFaction = Faction::MeridianCompact;
    for (const Entity& Entity : Scenario->Entities())
    {
        if (Entity.owner == UEchoesSimulationSubsystem::LocalPlayerId &&
            Entity.type == EntityType::Barracks && Entity.completed)
        {
            Foundry = Entity.id;
            LocalFaction = Entity.faction;
            break;
        }
    }
    const PlayerState* Local = Scenario->FindPlayer(UEchoesSimulationSubsystem::LocalPlayerId);
    const ResourcePool Cost = Scenario->ProductionCost(LocalFaction, EntityType::HeavyUnit);
    if (TestTrue(TEXT("The Scarce seat has a finished Foundry"), Foundry != 0) &&
        TestNotNull(TEXT("The Scarce seat exists"), Local) &&
        TestTrue(TEXT("Scarce cannot fund a heavy unit outright"),
            Local != nullptr && (Local->resources.material < Cost.material ||
                                 Local->resources.dawnshards < Cost.dawnshards)))
    {
        const ResourcePool Held = Local->resources;
        Feedback.Reset();
        TestFalse(TEXT("The unfunded heavy unit is refused before admission"),
            Bridge->IssueProductionCommand(Foundry, EntityType::HeavyUnit, Feedback));
        TestTrue(TEXT("The refusal names the short resource"),
            Feedback.StartsWith(Held.material < Cost.material
                ? TEXT("[INSUFFICIENT_MATTER]") : TEXT("[INSUFFICIENT_DAWN]")));
        TestTrue(TEXT("The refusal names the unit from the catalog"),
            Feedback.Contains(TEXT("BULWARK TEAM")));
        TestTrue(TEXT("The refusal states the price"),
            Feedback.Contains(FEchoesProductionReasonText::ProseCost(Cost)));
        TestTrue(TEXT("The refusal states the holding"),
            Feedback.Contains(FEchoesProductionReasonText::ProseCost(Held)));
        TestFalse(TEXT("The old blank refusal is gone"),
            Feedback.Contains(TEXT("cannot be funded")));
    }
    Bridge->StopPrototypeScenario();
    WorldWrapper.ForwardErrorMessages(this);
    return true;
}

#endif
