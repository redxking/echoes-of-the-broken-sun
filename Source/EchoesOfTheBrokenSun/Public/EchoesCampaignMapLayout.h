// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#pragma once

#include "CoreMinimal.h"
#include "EchoesCampaignProgress.h"
#include "EchoesPrologueMissionModel.h"

/** Visual and interaction state of a mission node on the Soryn Operations Map. */
enum class EEchoesCampaignNodeState : uint8
{
    Locked,
    Available,
    Completed
};

/** Screen layout geometry for a single mission node on the Operations Map. */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignMapNode final
{
    int32 Index = 0; // 0..14
    EEchoesCampaignMissionId MissionId = EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    EEchoesOperationMode Operation = EEchoesOperationMode::CampaignPrologue;
    FString MissionCode; // "M01".."M15"
    FString Title;
    FString BiomeName;
    FVector2D NormalizedPos = FVector2D::ZeroVector;
    FVector2D ScreenPos = FVector2D::ZeroVector;
    FBox2D Hitbox = FBox2D(ForceInit);
    float Radius = 18.0f;
    EEchoesCampaignNodeState State = EEchoesCampaignNodeState::Locked;
    echoes::sim::FutureWellChoice RecordedChoice = echoes::sim::FutureWellChoice::Dormant;
    int32 Act = 1;
};

/** Screen layout geometry for a corridor connecting two nodes. */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignMapCorridor final
{
    int32 FromNodeIndex = 0;
    int32 ToNodeIndex = 0;
    FVector2D StartPos = FVector2D::ZeroVector;
    FVector2D EndPos = FVector2D::ZeroVector;
    bool bActive = false;
};

/**
 * Pure layout structure for the in-engine Soryn Campaign Operations Map.
 * Computes resolution-independent hitboxes and layout bounds for all 15 campaign missions.
 */
struct ECHOESOFTHEBROKENSUN_API FEchoesCampaignMapLayout final
{
    FVector2D ViewportSize = FVector2D::ZeroVector;
    float HudScale = 1.0f;
    float ContentScale = 1.0f;
    float TextScale = 1.0f;

    FBox2D FullBounds = FBox2D(ForceInit);
    FBox2D TopRibbon = FBox2D(ForceInit);
    FBox2D MapCanvas = FBox2D(ForceInit);
    FBox2D InspectorDrawer = FBox2D(ForceInit);
    FBox2D BottomStatusBar = FBox2D(ForceInit);

    // Inspector controls
    FBox2D DeployButton = FBox2D(ForceInit);
    FBox2D BackButton = FBox2D(ForceInit);

    TArray<FEchoesCampaignMapNode> Nodes;
    TArray<FEchoesCampaignMapCorridor> Corridors;

    int32 CompletedMissionCount = 0;
    int32 ActiveMissionIndex = 0;

    /** Builds the resolution-independent campaign operations map geometry. */
    [[nodiscard]] static FEchoesCampaignMapLayout Build(
        const FVector2D& InViewportSize,
        float InHudScale,
        const FEchoesCampaignProgress& Progress,
        int32 SelectedNodeIndex = -1)
    {
        FEchoesCampaignMapLayout Layout;
        Layout.ViewportSize = FVector2D(
            FMath::Max(640.0f, InViewportSize.X),
            FMath::Max(480.0f, InViewportSize.Y));
        Layout.HudScale = FMath::Clamp(InHudScale, 0.75f, 1.35f);
        Layout.ContentScale = FMath::Clamp(
            FMath::Min(Layout.ViewportSize.X / 1920.0f, Layout.ViewportSize.Y / 1080.0f),
            0.65f,
            1.40f);
        Layout.TextScale = Layout.HudScale * Layout.ContentScale;

        Layout.FullBounds = FBox2D(
            FVector2D::ZeroVector,
            Layout.ViewportSize);

        const float Margin = 24.0f * Layout.ContentScale;
        const float TopRibbonHeight = 56.0f * Layout.ContentScale;
        const float BottomBarHeight = 44.0f * Layout.ContentScale;

        Layout.TopRibbon = FBox2D(
            FVector2D(Margin, Margin),
            FVector2D(Layout.ViewportSize.X - Margin, Margin + TopRibbonHeight));

        Layout.BottomStatusBar = FBox2D(
            FVector2D(Margin, Layout.ViewportSize.Y - Margin - BottomBarHeight),
            FVector2D(Layout.ViewportSize.X - Margin, Layout.ViewportSize.Y - Margin));

        const float UsableTop = Layout.TopRibbon.Max.Y + 12.0f * Layout.ContentScale;
        const float UsableBottom = Layout.BottomStatusBar.Min.Y - 12.0f * Layout.ContentScale;
        const float UsableHeight = UsableBottom - UsableTop;

        const float InspectorWidth = FMath::Clamp(
            Layout.ViewportSize.X * 0.30f,
            340.0f * Layout.ContentScale,
            460.0f * Layout.ContentScale);

        const float MapWidth = Layout.ViewportSize.X - (Margin * 2.0f) - InspectorWidth - (16.0f * Layout.ContentScale);

        Layout.MapCanvas = FBox2D(
            FVector2D(Margin, UsableTop),
            FVector2D(Margin + MapWidth, UsableBottom));

        Layout.InspectorDrawer = FBox2D(
            FVector2D(Layout.MapCanvas.Max.X + 16.0f * Layout.ContentScale, UsableTop),
            FVector2D(Layout.ViewportSize.X - Margin, UsableBottom));

        // Inspector Buttons
        const float ButtonHeight = 46.0f * Layout.ContentScale;
        const float InspectorMargin = 20.0f * Layout.ContentScale;
        const float InspectorLeft = Layout.InspectorDrawer.Min.X + InspectorMargin;
        const float InspectorRight = Layout.InspectorDrawer.Max.X - InspectorMargin;
        const float ButtonWidth = InspectorRight - InspectorLeft;

        const float BackY = Layout.InspectorDrawer.Max.Y - InspectorMargin - ButtonHeight;
        Layout.BackButton = FBox2D(
            FVector2D(InspectorLeft, BackY),
            FVector2D(InspectorRight, BackY + ButtonHeight));

        const float DeployY = BackY - 12.0f * Layout.ContentScale - ButtonHeight;
        Layout.DeployButton = FBox2D(
            FVector2D(InspectorLeft, DeployY),
            FVector2D(InspectorRight, DeployY + ButtonHeight));

        // Canonical 15 Mission Definitions with Normalized Coordinates
        struct FNodeDef
        {
            EEchoesCampaignMissionId MissionId;
            EEchoesOperationMode Operation;
            const TCHAR* Code;
            const TCHAR* Title;
            const TCHAR* Biome;
            float NormX;
            float NormY;
            int32 Act;
        };

        static const FNodeDef NodeDefs[15] = {
            { EEchoesCampaignMissionId::WhatTheLedgerKeeps, EEchoesOperationMode::CampaignPrologue, TEXT("M01"), TEXT("What the Ledger Keeps"), TEXT("Glass Scar Basin"), 0.16f, 0.44f, 1 },
            { EEchoesCampaignMissionId::SevenAccountsOfRain, EEchoesOperationMode::CampaignSevenAccounts, TEXT("M02"), TEXT("Seven Accounts of Rain"), TEXT("Shivergrass Steppe"), 0.28f, 0.28f, 1 },
            { EEchoesCampaignMissionId::ACityOnReserve, EEchoesOperationMode::CampaignCityReserve, TEXT("M03"), TEXT("A City on Reserve"), TEXT("ArkCity Foundry"), 0.46f, 0.24f, 1 },
            { EEchoesCampaignMissionId::TheUnburiedRoad, EEchoesOperationMode::CampaignUnburiedRoad, TEXT("M04"), TEXT("The Unburied Road"), TEXT("Subterranean Caverns"), 0.40f, 0.50f, 1 },
            { EEchoesCampaignMissionId::TermsOfContinuance, EEchoesOperationMode::CampaignTermsOfContinuance, TEXT("M05"), TEXT("Terms of Continuance"), TEXT("Lume Reach Refinery"), 0.64f, 0.36f, 1 },
            { EEchoesCampaignMissionId::NamesWithoutBirths, EEchoesOperationMode::CampaignNamesWithoutBirths, TEXT("M06"), TEXT("Names Without Births"), TEXT("Shivergrass Cryo-Vault"), 0.24f, 0.18f, 2 },
            { EEchoesCampaignMissionId::TheShapeOfSilence, EEchoesOperationMode::CampaignShapeOfSilence, TEXT("M07"), TEXT("The Shape of Silence"), TEXT("Echo Caverns"), 0.36f, 0.64f, 2 },
            { EEchoesCampaignMissionId::TheShapeBesideUs, EEchoesOperationMode::CampaignShapeBesideUs, TEXT("M08"), TEXT("The Shape Beside Us"), TEXT("Assembly Plaza"), 0.54f, 0.16f, 2 },
            { EEchoesCampaignMissionId::ReserveAuthority, EEchoesOperationMode::CampaignReserveAuthority, TEXT("M09"), TEXT("Reserve Authority"), TEXT("Glass Scar Relays"), 0.20f, 0.58f, 2 },
            { EEchoesCampaignMissionId::ChoirAtLumeReach, EEchoesOperationMode::CampaignChoirAtLumeReach, TEXT("M10"), TEXT("The Choir at Lume Reach"), TEXT("Lume Resonant Crater"), 0.72f, 0.46f, 2 },
            { EEchoesCampaignMissionId::NoNeutralLedger, EEchoesOperationMode::CampaignNoNeutralLedger, TEXT("M11"), TEXT("No Neutral Ledger"), TEXT("Grand Archives"), 0.58f, 0.28f, 3 },
            { EEchoesCampaignMissionId::TheFutureThatWon, EEchoesOperationMode::CampaignFutureThatWon, TEXT("M12"), TEXT("The Future That Won"), TEXT("Acoustic Monolith Fields"), 0.28f, 0.72f, 3 },
            { EEchoesCampaignMissionId::AssemblyOfTheMissing, EEchoesOperationMode::CampaignAssemblyOfTheMissing, TEXT("M13"), TEXT("Assembly of the Missing"), TEXT("Census Void Periphery"), 0.72f, 0.68f, 3 },
            { EEchoesCampaignMissionId::SeveralVoicesOneCommand, EEchoesOperationMode::CampaignSeveralVoicesOneCommand, TEXT("M14"), TEXT("Several Voices, One Command"), TEXT("Census Void Inner Sanctum"), 0.80f, 0.78f, 3 },
            { EEchoesCampaignMissionId::TheBrokenSun, EEchoesOperationMode::CampaignTheBrokenSun, TEXT("M15"), TEXT("The Broken Sun"), TEXT("Solar-Fall Dais"), 0.88f, 0.88f, 3 }
        };

        Layout.CompletedMissionCount = Progress.Decisions.Num();
        Layout.ActiveMissionIndex = FMath::Clamp(Layout.CompletedMissionCount, 0, 14);

        const float BaseRadius = 18.0f * Layout.ContentScale;
        Layout.Nodes.Reserve(15);

        for (int32 i = 0; i < 15; ++i)
        {
            const FNodeDef& Def = NodeDefs[i];
            FEchoesCampaignMapNode Node;
            Node.Index = i;
            Node.MissionId = Def.MissionId;
            Node.Operation = Def.Operation;
            Node.MissionCode = Def.Code;
            Node.Title = Def.Title;
            Node.BiomeName = Def.Biome;
            Node.NormalizedPos = FVector2D(Def.NormX, Def.NormY);
            Node.Act = Def.Act;
            Node.Radius = BaseRadius;

            // Absolute screen pos inside MapCanvas
            Node.ScreenPos = FVector2D(
                Layout.MapCanvas.Min.X + Def.NormX * Layout.MapCanvas.GetSize().X,
                Layout.MapCanvas.Min.Y + Def.NormY * Layout.MapCanvas.GetSize().Y);

            // Hitbox around node circle
            Node.Hitbox = FBox2D(
                Node.ScreenPos - FVector2D(BaseRadius * 1.35f, BaseRadius * 1.35f),
                Node.ScreenPos + FVector2D(BaseRadius * 1.35f, BaseRadius * 1.35f));

            // State resolution
            const FEchoesCampaignDecisionRecord* Decision = Progress.FindDecision(Def.MissionId);
            if (Decision != nullptr)
            {
                Node.State = EEchoesCampaignNodeState::Completed;
                Node.RecordedChoice = Decision->WellChoice;
            }
            else if (i == Layout.ActiveMissionIndex)
            {
                Node.State = EEchoesCampaignNodeState::Available;
            }
            else
            {
                Node.State = EEchoesCampaignNodeState::Locked;
            }

            Layout.Nodes.Add(Node);
        }

        // Build connecting corridors
        // 1. Sequential campaign mainline corridors (M01 -> M02 -> ... -> M15)
        Layout.Corridors.Reserve(20);
        for (int32 i = 0; i < 14; ++i)
        {
            FEchoesCampaignMapCorridor Corridor;
            Corridor.FromNodeIndex = i;
            Corridor.ToNodeIndex = i + 1;
            Corridor.StartPos = Layout.Nodes[i].ScreenPos;
            Corridor.EndPos = Layout.Nodes[i + 1].ScreenPos;
            Corridor.bActive = (i < Layout.CompletedMissionCount);
            Layout.Corridors.Add(Corridor);
        }

        // 2. Regional biome branch corridors
        const auto AddBiomeLink = [&](int32 From, int32 To)
        {
            FEchoesCampaignMapCorridor Corridor;
            Corridor.FromNodeIndex = From;
            Corridor.ToNodeIndex = To;
            Corridor.StartPos = Layout.Nodes[From].ScreenPos;
            Corridor.EndPos = Layout.Nodes[To].ScreenPos;
            Corridor.bActive = (Layout.Nodes[From].State == EEchoesCampaignNodeState::Completed &&
                                Layout.Nodes[To].State != EEchoesCampaignNodeState::Locked);
            Layout.Corridors.Add(Corridor);
        };

        AddBiomeLink(1, 5);  // Shivergrass: M02 -> M06
        AddBiomeLink(2, 7);  // ArkCity: M03 -> M08
        AddBiomeLink(7, 10); // ArkCity: M08 -> M11
        AddBiomeLink(3, 6);  // Unburied: M04 -> M07
        AddBiomeLink(4, 9);  // Lume Reach: M05 -> M10
        AddBiomeLink(0, 8);  // Glass Scar: M01 -> M09
        AddBiomeLink(8, 11); // Glass Scar: M09 -> M12

        return Layout;
    }

    /** Hit tests against the 15 mission nodes and inspector buttons. */
    [[nodiscard]] int32 HitTestNode(const FVector2D& ScreenPosition) const
    {
        for (int32 i = 0; i < Nodes.Num(); ++i)
        {
            if (Nodes[i].Hitbox.IsInsideOrOn(ScreenPosition))
            {
                return i;
            }
        }
        return -1;
    }

    [[nodiscard]] bool HitTestDeploy(const FVector2D& ScreenPosition) const
    {
        return DeployButton.IsInsideOrOn(ScreenPosition);
    }

    [[nodiscard]] bool HitTestBack(const FVector2D& ScreenPosition) const
    {
        return BackButton.IsInsideOrOn(ScreenPosition);
    }
};
