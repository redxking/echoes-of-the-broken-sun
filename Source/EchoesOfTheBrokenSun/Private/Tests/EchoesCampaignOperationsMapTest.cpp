// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "EchoesTestSaveEnvironment.h"
#include "EchoesCampaignMapLayout.h"
#include "EchoesCampaignRewards.h"
#include "EchoesCampaignProgress.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEchoesCampaignOperationsMapTest,
    "Echoes.Runtime.Campaign.OperationsMap",
    EAutomationTestFlags::EditorContext |
        EAutomationTestFlags::ClientContext |
        EAutomationTestFlags::EngineFilter)

bool FEchoesCampaignOperationsMapTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FEchoesScopedTestSaveEnvironment TestSaveEnvironment(*this);
    if (!TestSaveEnvironment.IsReady())
    {
        return false;
    }

    // 1. Test Viewport Geometry & Resolution Independence
    const FVector2D TestResolutions[] = {
        FVector2D(1280.0f, 720.0f),   // 720p 16:9
        FVector2D(1920.0f, 1080.0f),  // 1080p 16:9
        FVector2D(2560.0f, 1440.0f),  // 1440p 16:9
        FVector2D(1920.0f, 1200.0f),  // 16:10
        FVector2D(3440.0f, 1440.0f)   // 21:9 Ultrawide
    };

    FEchoesCampaignProgress EmptyProgress;

    for (const FVector2D& Res : TestResolutions)
    {
        const FEchoesCampaignMapLayout Layout = FEchoesCampaignMapLayout::Build(
            Res, 1.0f, EmptyProgress, 0);

        TestTrue(
            *FString::Printf(TEXT("Resolution %dx%d produces valid map canvas"),
                             static_cast<int32>(Res.X), static_cast<int32>(Res.Y)),
            Layout.MapCanvas.GetSize().X > 300.0f && Layout.MapCanvas.GetSize().Y > 200.0f);

        TestTrue(
            *FString::Printf(TEXT("Resolution %dx%d produces valid inspector drawer"),
                             static_cast<int32>(Res.X), static_cast<int32>(Res.Y)),
            Layout.InspectorDrawer.GetSize().X > 200.0f && Layout.InspectorDrawer.GetSize().Y > 200.0f);

        TestEqual(
            *FString::Printf(TEXT("Resolution %dx%d lays out exactly 15 mission nodes"),
                             static_cast<int32>(Res.X), static_cast<int32>(Res.Y)),
            Layout.Nodes.Num(), 15);

        TestTrue(
            *FString::Printf(TEXT("Resolution %dx%d produces mainline corridors"),
                             static_cast<int32>(Res.X), static_cast<int32>(Res.Y)),
            Layout.Corridors.Num() >= 14);
    }

    // 2. Node Coordinates and Non-Overlap Verification
    const FEchoesCampaignMapLayout StandardLayout = FEchoesCampaignMapLayout::Build(
        FVector2D(1920.0f, 1080.0f), 1.0f, EmptyProgress, 0);

    for (int32 i = 0; i < StandardLayout.Nodes.Num(); ++i)
    {
        const FEchoesCampaignMapNode& NodeA = StandardLayout.Nodes[i];

        // Ensure strictly inside MapCanvas
        TestTrue(
            *FString::Printf(TEXT("Node %s is positioned within map canvas"), *NodeA.MissionCode),
            StandardLayout.MapCanvas.IsInsideOrOn(NodeA.ScreenPos));

        // Ensure clearance with all other nodes
        for (int32 j = i + 1; j < StandardLayout.Nodes.Num(); ++j)
        {
            const FEchoesCampaignMapNode& NodeB = StandardLayout.Nodes[j];
            const float Distance = FVector2D::Distance(NodeA.ScreenPos, NodeB.ScreenPos);
            TestTrue(
                *FString::Printf(TEXT("Nodes %s and %s have clearance (dist=%.1f >= 32.0)"),
                                 *NodeA.MissionCode, *NodeB.MissionCode, Distance),
                Distance >= 32.0f);
        }
    }

    // 3. Ledger State Projection (Empty, Mid-Campaign, Complete)
    // Empty Ledger
    TestEqual(TEXT("M01 is Available in fresh campaign"),
              StandardLayout.Nodes[0].State, EEchoesCampaignNodeState::Available);
    for (int32 i = 1; i < 15; ++i)
    {
        TestEqual(*FString::Printf(TEXT("Node %d is Locked in fresh campaign"), i + 1),
                  StandardLayout.Nodes[i].State, EEchoesCampaignNodeState::Locked);
    }

    // Mid-Campaign (5 decisions recorded)
    FEchoesCampaignProgress MidProgress;
    for (int32 i = 1; i <= 5; ++i)
    {
        FEchoesCampaignDecisionRecord Record;
        Record.Mission = static_cast<EEchoesCampaignMissionId>(i);
        Record.WellChoice = echoes::sim::FutureWellChoice::Harvest;
        MidProgress.Decisions.Add(Record);
    }

    const FEchoesCampaignMapLayout MidLayout = FEchoesCampaignMapLayout::Build(
        FVector2D(1920.0f, 1080.0f), 1.0f, MidProgress, 5);

    for (int32 i = 0; i < 5; ++i)
    {
        TestEqual(*FString::Printf(TEXT("Node %d is Completed in mid-campaign"), i + 1),
                  MidLayout.Nodes[i].State, EEchoesCampaignNodeState::Completed);
    }
    TestEqual(TEXT("Node 6 (M06) is Available in mid-campaign"),
              MidLayout.Nodes[5].State, EEchoesCampaignNodeState::Available);
    for (int32 i = 6; i < 15; ++i)
    {
        TestEqual(*FString::Printf(TEXT("Node %d is Locked in mid-campaign"), i + 1),
                  MidLayout.Nodes[i].State, EEchoesCampaignNodeState::Locked);
    }

    // Full 15-Mission Complete
    FEchoesCampaignProgress FullProgress;
    for (int32 i = 1; i <= 15; ++i)
    {
        FEchoesCampaignDecisionRecord Record;
        Record.Mission = static_cast<EEchoesCampaignMissionId>(i);
        Record.WellChoice = echoes::sim::FutureWellChoice::Preserve;
        FullProgress.Decisions.Add(Record);
    }

    const FEchoesCampaignMapLayout FullLayout = FEchoesCampaignMapLayout::Build(
        FVector2D(1920.0f, 1080.0f), 1.0f, FullProgress, 14);

    TestEqual(TEXT("Full progress ledger has 15 completed missions"),
              FullLayout.CompletedMissionCount, 15);

    for (int32 i = 0; i < 15; ++i)
    {
        TestEqual(*FString::Printf(TEXT("Node %d is Completed when campaign finished"), i + 1),
                  FullLayout.Nodes[i].State, EEchoesCampaignNodeState::Completed);
    }

    // 4. Hit Testing
    for (int32 i = 0; i < StandardLayout.Nodes.Num(); ++i)
    {
        const int32 Hit = StandardLayout.HitTestNode(StandardLayout.Nodes[i].ScreenPos);
        TestEqual(*FString::Printf(TEXT("Hit-testing node %d center resolves index"), i),
                  Hit, i);
    }

    const FVector2D DeployCenter = (StandardLayout.DeployButton.Min + StandardLayout.DeployButton.Max) * 0.5f;
    TestTrue(TEXT("Deploy button hit-test succeeds"),
             StandardLayout.HitTestDeploy(DeployCenter));

    const FVector2D BackCenter = (StandardLayout.BackButton.Min + StandardLayout.BackButton.Max) * 0.5f;
    TestTrue(TEXT("Back button hit-test succeeds"),
             StandardLayout.HitTestBack(BackCenter));

    // 5. Campaign Persistent Rewards Contract
    const TArray<FEchoesMissionReward> AllRewards = FEchoesCampaignRewards::GetAllRewards();
    TestEqual(TEXT("Authored rewards manifest contains exactly 15 mission definitions"),
              AllRewards.Num(), 15);

    for (const FEchoesMissionReward& Reward : AllRewards)
    {
        TestFalse(*FString::Printf(TEXT("Reward %s has non-empty Skirmish unlock"), *Reward.MissionCode),
                  Reward.SkirmishMapUnlock.IsEmpty());
        TestFalse(*FString::Printf(TEXT("Reward %s has non-empty Doctrine unlock"), *Reward.MissionCode),
                  Reward.DoctrineUnlock.IsEmpty());
        TestFalse(*FString::Printf(TEXT("Reward %s has non-empty Codex unlock"), *Reward.MissionCode),
                  Reward.CodexUnlock.IsEmpty());
    }

    const TArray<FEchoesMissionReward> MidUnlocked = FEchoesCampaignRewards::GetUnlockedRewards(MidProgress);
    TestEqual(TEXT("Mid-campaign unlocks exactly 5 rewards"), MidUnlocked.Num(), 5);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
