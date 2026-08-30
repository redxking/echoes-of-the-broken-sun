#pragma once

#include "CoreMinimal.h"
#include "EchoesSimCore/Simulation.h"

namespace echoes::presentation
{
struct FFactionTechnologyProfile final
{
    sim::ResearchType TierOne = sim::ResearchType::None;
    sim::ResearchType TierTwo = sim::ResearchType::None;
    const TCHAR* TierOneContentId = TEXT("");
    const TCHAR* TierTwoContentId = TEXT("");
};

[[nodiscard]] inline const TCHAR* FactionDisplayName(sim::Faction Faction)
{
    switch (Faction)
    {
        case sim::Faction::MeridianCompact:
            return TEXT("MERIDIAN COMPACT");
        case sim::Faction::KharuunAssemblies:
            return TEXT("KHARUUN ASSEMBLIES");
        case sim::Faction::HollowChoir:
            return TEXT("HOLLOW CHOIR");
    }
    return TEXT("UNKNOWN FACTION");
}

[[nodiscard]] inline const TCHAR* FactionShortName(sim::Faction Faction)
{
    switch (Faction)
    {
        case sim::Faction::MeridianCompact:
            return TEXT("MERIDIAN");
        case sim::Faction::KharuunAssemblies:
            return TEXT("KHARUUN");
        case sim::Faction::HollowChoir:
            return TEXT("CHOIR");
    }
    return TEXT("UNKNOWN");
}

/** Stable 1v1 skirmish matchup policy; campaigns retain authored authority. */
[[nodiscard]] inline sim::Faction SkirmishOpponent(sim::Faction LocalFaction)
{
    switch (LocalFaction)
    {
        case sim::Faction::MeridianCompact:
            return sim::Faction::KharuunAssemblies;
        case sim::Faction::KharuunAssemblies:
        case sim::Faction::HollowChoir:
            return sim::Faction::MeridianCompact;
    }
    return sim::Faction::MeridianCompact;
}

[[nodiscard]] inline sim::Faction NextPlayableFaction(sim::Faction Faction)
{
    switch (Faction)
    {
        case sim::Faction::MeridianCompact:
            return sim::Faction::KharuunAssemblies;
        case sim::Faction::KharuunAssemblies:
            return sim::Faction::HollowChoir;
        case sim::Faction::HollowChoir:
            return sim::Faction::MeridianCompact;
    }
    return sim::Faction::MeridianCompact;
}

[[nodiscard]] inline FFactionTechnologyProfile TechnologyProfile(
    sim::Faction Faction)
{
    switch (Faction)
    {
        case sim::Faction::MeridianCompact:
            return {
                sim::ResearchType::MeridianPrismaticTargeting,
                sim::ResearchType::MeridianHorizonLattice,
                TEXT("mc_prismatic_targeting"),
                TEXT("mc_horizon_lattice")};
        case sim::Faction::KharuunAssemblies:
            return {
                sim::ResearchType::KharuunEchoCartography,
                sim::ResearchType::KharuunAncestralEdge,
                TEXT("ka_echo_cartography"),
                TEXT("ka_ancestral_edge")};
        case sim::Faction::HollowChoir:
            return {
                sim::ResearchType::ChoirHeldAlternatives,
                sim::ResearchType::ChoirSharedResolution,
                TEXT("hc_held_alternatives"),
                TEXT("hc_shared_resolution")};
    }
    return {};
}
}
