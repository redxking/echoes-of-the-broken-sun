// Copyright Echoes of the Broken Sun. All Rights Reserved.
// Author: Angelis Pseftis

#include "EchoesCampaignRewards.h"

namespace
{
const FEchoesMissionReward StaticRewards[] = {
    {
        EEchoesCampaignMissionId::WhatTheLedgerKeeps,
        TEXT("M01"),
        TEXT("What the Ledger Keeps"),
        TEXT("Vitrified Basin"),
        TEXT("Glass Scar - Vitrified Basin"),
        TEXT("meridian_compact"),
        TEXT("Meridian Scout Drone Recon Relay"),
        TEXT("The Displaced Archive of Talar Venn"),
        TEXT("Unlocks Glass Scar in Skirmish, Scout Drone telemetry, and Master Ledger Ch. 1.")
    },
    {
        EEchoesCampaignMissionId::SevenAccountsOfRain,
        TEXT("M02"),
        TEXT("Seven Accounts of Rain"),
        TEXT("Shivergrass Steppe"),
        TEXT("Shivergrass Steppe"),
        TEXT("kharuun_assemblies"),
        TEXT("Kharuun Mobile Waystone Migration Anchor"),
        TEXT("Seven Accounts of Rain - The Oral Archive"),
        TEXT("Unlocks Shivergrass Steppe, Kharuun Assemblies faction in Skirmish, and oral lore.")
    },
    {
        EEchoesCampaignMissionId::ACityOnReserve,
        TEXT("M03"),
        TEXT("A City on Reserve"),
        TEXT("ArkCity Foundry"),
        TEXT("ArkCity Verge Foundry"),
        TEXT("none"),
        TEXT("Emergency District Grid Overcharge"),
        TEXT("Lume Reach Structural Census Log 03"),
        TEXT("Unlocks ArkCity Verge Foundry skirmish map, Emergency Grid Overcharge, and reserve schematics.")
    },
    {
        EEchoesCampaignMissionId::TheUnburiedRoad,
        TEXT("M04"),
        TEXT("The Unburied Road"),
        TEXT("Subterranean Caverns"),
        TEXT("The Unburied Highway"),
        TEXT("none"),
        TEXT("Subterranean Sapper Trenching"),
        TEXT("Pre-Crownfall Transit Arteries"),
        TEXT("Unlocks Subterranean Highway map, Sapper Trenching doctrine, and geological surveys.")
    },
    {
        EEchoesCampaignMissionId::TermsOfContinuance,
        TEXT("M05"),
        TEXT("Terms of Continuance"),
        TEXT("Lume Reach Refinery"),
        TEXT("Lume Reach Refinery Verge"),
        TEXT("none"),
        TEXT("Meridian Treaty Proxy Synchronization"),
        TEXT("The Ceasefire Accord of the Outer Spire"),
        TEXT("Unlocks Refinery Verge map, Treaty Proxy Synchronization, and diplomatic records.")
    },
    {
        EEchoesCampaignMissionId::NamesWithoutBirths,
        TEXT("M06"),
        TEXT("Names Without Births"),
        TEXT("Shivergrass Cryo-Vault"),
        TEXT("Cryo-Vault Expanse"),
        TEXT("none"),
        TEXT("Civilian Emergency Shelter Protocol"),
        TEXT("The Unregistered Citizen Index"),
        TEXT("Unlocks Cryo-Vault Expanse arena, Emergency Shelter protocol, and unlisted citizen logs.")
    },
    {
        EEchoesCampaignMissionId::TheShapeOfSilence,
        TEXT("M07"),
        TEXT("The Shape of Silence"),
        TEXT("Echo Caverns"),
        TEXT("Echo Caverns Confluence"),
        TEXT("none"),
        TEXT("Listening Spine Harmonic Resonance"),
        TEXT("Communal Memory Hollow Observations"),
        TEXT("Unlocks Echo Caverns map, Listening Spine Harmonic Resonance, and communal memory logs.")
    },
    {
        EEchoesCampaignMissionId::TheShapeBesideUs,
        TEXT("M08"),
        TEXT("The Shape Beside Us"),
        TEXT("Assembly Plaza"),
        TEXT("Assembly Plaza Quadrant"),
        TEXT("none"),
        TEXT("Dual-State Traversal Calibration"),
        TEXT("Hollow Choir First Reciprocal Contact"),
        TEXT("Unlocks Assembly Plaza map, Dual-State Traversal, and early Hollow Choir acoustics.")
    },
    {
        EEchoesCampaignMissionId::ReserveAuthority,
        TEXT("M09"),
        TEXT("Reserve Authority"),
        TEXT("Glass Scar Relays"),
        TEXT("Solar Pylon Array Relay"),
        TEXT("none"),
        TEXT("Solar Pylon Power Channeling"),
        TEXT("Meridian Reserve Energy Allocation Act"),
        TEXT("Unlocks Solar Pylon Relay map, Solar Pylon Channeling, and the central power grid registry.")
    },
    {
        EEchoesCampaignMissionId::ChoirAtLumeReach,
        TEXT("M10"),
        TEXT("The Choir at Lume Reach"),
        TEXT("Lume Resonant Crater"),
        TEXT("Lume Resonant Caldera"),
        TEXT("hollow_choir"),
        TEXT("Phase Shift & Possibility Entanglement"),
        TEXT("The Lume Reach Harmonic Synthesis"),
        TEXT("Unlocks Hollow Choir as a playable skirmish faction, Phase Shift, and Lume Synthesis logs.")
    },
    {
        EEchoesCampaignMissionId::NoNeutralLedger,
        TEXT("M11"),
        TEXT("No Neutral Ledger"),
        TEXT("Grand Archives"),
        TEXT("Grand Archives Sector"),
        TEXT("none"),
        TEXT("Public Evidence Attestation Network"),
        TEXT("Soryn Cross-Faction Coalition Treaty"),
        TEXT("Unlocks Grand Archives map, Public Evidence Attestation, and Coalition Treaty documents.")
    },
    {
        EEchoesCampaignMissionId::TheFutureThatWon,
        TEXT("M12"),
        TEXT("The Future That Won"),
        TEXT("Acoustic Monolith Fields"),
        TEXT("Acoustic Monolith Fields"),
        TEXT("none"),
        TEXT("Independent Public Readback Verification"),
        TEXT("The Demonstrator Branch Ledger"),
        TEXT("Unlocks Monolith Fields map, Independent Readback, and historical alternate branch logs.")
    },
    {
        EEchoesCampaignMissionId::AssemblyOfTheMissing,
        TEXT("M13"),
        TEXT("Assembly of the Missing"),
        TEXT("Census Void Periphery"),
        TEXT("Census Void Outer Rim"),
        TEXT("none"),
        TEXT("Crownfall Index Link Calibration"),
        TEXT("The Roster of the Uncounted"),
        TEXT("Unlocks Census Void Outer Rim map, Crownfall Index Link, and memorial records.")
    },
    {
        EEchoesCampaignMissionId::SeveralVoicesOneCommand,
        TEXT("M14"),
        TEXT("Several Voices, One Command"),
        TEXT("Census Void Inner Sanctum"),
        TEXT("Census Void Core Sanctum"),
        TEXT("none"),
        TEXT("Phase Anchor Crisis Stabilization"),
        TEXT("Choir Polyphonic Synthesis Directive"),
        TEXT("Unlocks Core Sanctum map, Phase Anchor Crisis Stabilization, and Choir polyphonic manuscripts.")
    },
    {
        EEchoesCampaignMissionId::TheBrokenSun,
        TEXT("M15"),
        TEXT("The Broken Sun"),
        TEXT("Solar-Fall Dais"),
        TEXT("The Broken Sun Coronal Dais"),
        TEXT("all_doctrines_mastered"),
        TEXT("Soryn Resolution Conduit Array"),
        TEXT("Four Canonical Soryn Epilogues"),
        TEXT("Unlocks The Broken Sun Dais in Skirmish, Resolution Conduit Array, and all 4 canonical epilogues.")
    }
};

EEchoesCampaignMissionId MissionIdFromOperation(EEchoesOperationMode Operation)
{
    switch (Operation)
    {
        case EEchoesOperationMode::CampaignPrologue:
            return EEchoesCampaignMissionId::WhatTheLedgerKeeps;
        case EEchoesOperationMode::CampaignSevenAccounts:
            return EEchoesCampaignMissionId::SevenAccountsOfRain;
        case EEchoesOperationMode::CampaignCityReserve:
            return EEchoesCampaignMissionId::ACityOnReserve;
        case EEchoesOperationMode::CampaignUnburiedRoad:
            return EEchoesCampaignMissionId::TheUnburiedRoad;
        case EEchoesOperationMode::CampaignTermsOfContinuance:
            return EEchoesCampaignMissionId::TermsOfContinuance;
        case EEchoesOperationMode::CampaignNamesWithoutBirths:
            return EEchoesCampaignMissionId::NamesWithoutBirths;
        case EEchoesOperationMode::CampaignShapeOfSilence:
            return EEchoesCampaignMissionId::TheShapeOfSilence;
        case EEchoesOperationMode::CampaignShapeBesideUs:
            return EEchoesCampaignMissionId::TheShapeBesideUs;
        case EEchoesOperationMode::CampaignReserveAuthority:
            return EEchoesCampaignMissionId::ReserveAuthority;
        case EEchoesOperationMode::CampaignChoirAtLumeReach:
            return EEchoesCampaignMissionId::ChoirAtLumeReach;
        case EEchoesOperationMode::CampaignNoNeutralLedger:
            return EEchoesCampaignMissionId::NoNeutralLedger;
        case EEchoesOperationMode::CampaignFutureThatWon:
            return EEchoesCampaignMissionId::TheFutureThatWon;
        case EEchoesOperationMode::CampaignAssemblyOfTheMissing:
            return EEchoesCampaignMissionId::AssemblyOfTheMissing;
        case EEchoesOperationMode::CampaignSeveralVoicesOneCommand:
            return EEchoesCampaignMissionId::SeveralVoicesOneCommand;
        case EEchoesOperationMode::CampaignTheBrokenSun:
            return EEchoesCampaignMissionId::TheBrokenSun;
        default:
            return EEchoesCampaignMissionId::WhatTheLedgerKeeps;
    }
}
} // namespace

const FEchoesMissionReward* FEchoesCampaignRewards::GetReward(
    EEchoesCampaignMissionId MissionId)
{
    const uint8 Index = static_cast<uint8>(MissionId);
    if (Index >= 1 && Index <= UE_ARRAY_COUNT(StaticRewards))
    {
        return &StaticRewards[Index - 1];
    }
    return nullptr;
}

const FEchoesMissionReward* FEchoesCampaignRewards::GetRewardForOperation(
    EEchoesOperationMode Operation)
{
    return GetReward(MissionIdFromOperation(Operation));
}

TArray<FEchoesMissionReward> FEchoesCampaignRewards::GetAllRewards()
{
    TArray<FEchoesMissionReward> Out;
    Out.Append(StaticRewards, UE_ARRAY_COUNT(StaticRewards));
    return Out;
}

TArray<FEchoesMissionReward> FEchoesCampaignRewards::GetUnlockedRewards(
    const FEchoesCampaignProgress& Progress)
{
    TArray<FEchoesMissionReward> Out;
    for (const FEchoesCampaignDecisionRecord& Decision : Progress.Decisions)
    {
        const FEchoesMissionReward* Reward = GetReward(Decision.Mission);
        if (Reward != nullptr)
        {
            Out.Add(*Reward);
        }
    }
    return Out;
}

bool FEchoesCampaignRewards::IsRewardUnlocked(
    EEchoesCampaignMissionId MissionId,
    const FEchoesCampaignProgress& Progress)
{
    return Progress.FindDecision(MissionId) != nullptr;
}
