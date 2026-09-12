#include "EchoesSimCore/Simulation.h"
#include "EchoesSimCore/SkirmishMapPresets.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <cstdio>
#include <map>
#include <set>
#include <thread>
#include <vector>

namespace echoes::balance {

using namespace echoes::sim;

// WI-6/SPEC-BAL-001/003/007: the harness has to measure the ruleset the game
// actually builds. DefaultSimulationRules is not that ruleset -- it gives the
// Meridian Lancer 120 health and a 12-tick attack period, where
// Content/Data/Source/units.json authors 145 health and 30 ticks, so every
// balance number taken from the defaults described a game nobody can play.
// These rules are loaded from the same authored source the shipped catalog
// compiles, using the same conversions as the runtime adapter, and the loader
// fails closed: a missing or malformed field aborts the run rather than
// silently falling back to the defaults this exists to replace.
struct ContentRuleLoad final {
    SimulationRules rules{};
    std::string source = "content-data";
    std::uint64_t archetypeChecksum = 0;
    std::int32_t lancerMaxHealth = 0;
    Tick lancerCooldownTicks = 0;
};

[[noreturn]] void ContentRuleFailure(const std::string& detail) {
    std::cerr << "AiBalanceHarness: authored content rules unusable: " << detail
              << "\n";
    std::exit(3);
}

std::string ReadSourceFile(const std::string& relative) {
    std::string here = __FILE__;
    const std::size_t cut = here.find_last_of("/\\");
    const std::string root =
        (cut == std::string::npos ? std::string(".") : here.substr(0, cut)) +
        "/../..";
    const std::string path = root + "/" + relative;
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) {
        ContentRuleFailure("cannot open " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Records in these authored files are one flat JSON object per line, so a
// bounded scan over a record is enough; anything unexpected is fatal.
std::vector<std::string> SplitRecords(const std::string& text,
                                      const std::string& arrayKey) {
    const std::size_t start = text.find("\"" + arrayKey + "\"");
    if (start == std::string::npos) {
        ContentRuleFailure("array " + arrayKey + " missing");
    }
    std::vector<std::string> records;
    std::size_t cursor = start;
    while (true) {
        const std::size_t open = text.find('{', cursor);
        if (open == std::string::npos) {
            break;
        }
        // Depth-aware: a record carries nested objects (cost, attack), so the
        // first closing brace is not the end of the record.
        std::size_t depth = 0;
        std::size_t close = std::string::npos;
        for (std::size_t scan = open; scan < text.size(); ++scan) {
            if (text[scan] == '{') {
                ++depth;
            } else if (text[scan] == '}') {
                --depth;
                if (depth == 0) {
                    close = scan;
                    break;
                }
            }
        }
        if (close == std::string::npos) {
            ContentRuleFailure("unterminated record in " + arrayKey);
        }
        std::string record = text.substr(open, close - open + 1);
        if (record.find("\"id\"") != std::string::npos) {
            records.push_back(record);
        }
        cursor = close + 1;
    }
    if (records.empty()) {
        ContentRuleFailure("no records in " + arrayKey);
    }
    return records;
}

std::string RecordId(const std::string& record) {
    const std::size_t key = record.find("\"id\"");
    if (key == std::string::npos) {
        ContentRuleFailure("record without id");
    }
    const std::size_t first = record.find('"', record.find(':', key) + 1);
    const std::size_t last = record.find('"', first + 1);
    if (first == std::string::npos || last == std::string::npos) {
        ContentRuleFailure("unreadable id");
    }
    return record.substr(first + 1, last - first - 1);
}

std::int64_t RequireInt(const std::string& record,
                        const std::string& key,
                        const std::string& scope) {
    const std::size_t at = record.find("\"" + key + "\"");
    if (at == std::string::npos) {
        ContentRuleFailure(scope + " is missing " + key);
    }
    std::size_t cursor = record.find(':', at);
    if (cursor == std::string::npos) {
        ContentRuleFailure(scope + " has no value for " + key);
    }
    ++cursor;
    // Tolerate a leading array bracket: footprint_cells is authored as [w, h]
    // and its first element is the span this loader needs.
    while (cursor < record.size() &&
           (record[cursor] == ' ' || record[cursor] == '[')) {
        ++cursor;
    }
    bool negative = false;
    if (cursor < record.size() && record[cursor] == '-') {
        negative = true;
        ++cursor;
    }
    if (cursor >= record.size() || record[cursor] < '0' || record[cursor] > '9') {
        ContentRuleFailure(scope + " value for " + key + " is not an integer");
    }
    std::int64_t value = 0;
    while (cursor < record.size() && record[cursor] >= '0' &&
           record[cursor] <= '9') {
        value = value * 10 + (record[cursor] - '0');
        ++cursor;
    }
    return negative ? -value : value;
}

bool HasKey(const std::string& record, const std::string& key) {
    return record.find("\"" + key + "\"") != std::string::npos;
}

ContentRuleLoad LoadAuthoredRules(std::uint32_t ticksPerSecond) {
    ContentRuleLoad load{};
    load.rules = DefaultSimulationRules();

    struct Binding final {
        const char* id;
        Faction faction;
        EntityType type;
    };
    // The same twelve unit and twelve building bindings the runtime catalog uses.
    static const std::array<Binding, 12> unitBindings{{
        {"mc_surveyor", Faction::MeridianCompact, EntityType::Worker},
        {"mc_lancer", Faction::MeridianCompact, EntityType::Soldier},
        {"mc_bulwark_team", Faction::MeridianCompact, EntityType::HeavyUnit},
        {"mc_relay_skiff", Faction::MeridianCompact, EntityType::ScoutUnit},
        {"ka_tender", Faction::KharuunAssemblies, EntityType::Worker},
        {"ka_riftstalker", Faction::KharuunAssemblies, EntityType::Soldier},
        {"ka_cairnback", Faction::KharuunAssemblies, EntityType::HeavyUnit},
        {"ka_resonant", Faction::KharuunAssemblies, EntityType::ScoutUnit},
        {"hc_threadkeeper", Faction::HollowChoir, EntityType::Worker},
        {"hc_intervalist", Faction::HollowChoir, EntityType::Soldier},
        {"hc_lacuna_warden", Faction::HollowChoir, EntityType::HeavyUnit},
        {"hc_afterimage", Faction::HollowChoir, EntityType::ScoutUnit},
    }};

    const std::string unitText = ReadSourceFile("Content/Data/Source/units.json");
    std::int32_t bound = 0;
    for (const std::string& record : SplitRecords(unitText, "units")) {
        const std::string id = RecordId(record);
        const Binding* binding = nullptr;
        for (const Binding& candidate : unitBindings) {
            if (id == candidate.id) {
                binding = &candidate;
                break;
            }
        }
        if (binding == nullptr) {
            continue;
        }
        EntityArchetypeRules& archetype =
            load.rules.archetypes[static_cast<std::size_t>(binding->faction)]
                                 [static_cast<std::size_t>(binding->type)];
        archetype.cost = {
            static_cast<std::int32_t>(RequireInt(record, "matter", id)),
            static_cast<std::int32_t>(RequireInt(record, "dawn", id))};
        archetype.maxHitPoints =
            static_cast<std::int32_t>(RequireInt(record, "max_health", id));
        const std::int64_t speed = RequireInt(record, "move_speed_cm_s", id);
        archetype.movementPerTickRaw = static_cast<std::int32_t>(
            speed * kFixedScale / (static_cast<std::int64_t>(ticksPerSecond) * 100));
        if (archetype.movementPerTickRaw <= 0) {
            ContentRuleFailure(id + " has no usable movement");
        }
        const std::int64_t sight = RequireInt(record, "sight_cm", id);
        archetype.visionTiles = static_cast<std::int32_t>((sight + 99) / 100);
        archetype.populationCost =
            static_cast<std::int32_t>(RequireInt(record, "population_cost", id));
        archetype.productionTicks =
            static_cast<std::int32_t>(RequireInt(record, "production_ticks", id));
        archetype.workRate = HasKey(record, "work_rate")
            ? static_cast<std::int32_t>(RequireInt(record, "work_rate", id))
            : 0;
        archetype.cargoCapacity = HasKey(record, "cargo_capacity")
            ? static_cast<std::int32_t>(RequireInt(record, "cargo_capacity", id))
            : 0;
        if (HasKey(record, "attack")) {
            archetype.attackDamage =
                static_cast<std::int32_t>(RequireInt(record, "damage", id));
            archetype.attackRangeRaw = static_cast<std::int32_t>(
                RequireInt(record, "range_cm", id) * kFixedScale / 100);
            archetype.attackPeriodTicks = static_cast<Tick>(
                RequireInt(record, "cooldown_ticks", id));
        } else {
            archetype.attackDamage = 0;
            archetype.attackRangeRaw = 0;
            archetype.attackPeriodTicks = 0;
        }
        archetype.footprintHalfExtentRaw = kFixedScale / 8;
        if (id == "mc_lancer") {
            load.lancerMaxHealth = archetype.maxHitPoints;
            load.lancerCooldownTicks = archetype.attackPeriodTicks;
        }
        ++bound;
    }
    if (bound != static_cast<std::int32_t>(unitBindings.size())) {
        ContentRuleFailure("bound " + std::to_string(bound) +
                           " authored units, expected 12");
    }

    static const std::array<Binding, 12> buildingBindings{{
        {"mc_anchor", Faction::MeridianCompact, EntityType::CommandCore},
        {"mc_power_link", Faction::MeridianCompact, EntityType::Dropoff},
        {"mc_array_foundry", Faction::MeridianCompact, EntityType::Barracks},
        {"mc_aegis_post", Faction::MeridianCompact, EntityType::UtilityStructure},
        {"ka_memory_hearth", Faction::KharuunAssemblies, EntityType::CommandCore},
        {"ka_waystone", Faction::KharuunAssemblies, EntityType::Dropoff},
        {"ka_growth_basin", Faction::KharuunAssemblies, EntityType::Barracks},
        {"ka_listening_spine", Faction::KharuunAssemblies, EntityType::UtilityStructure},
        {"hc_concordance", Faction::HollowChoir, EntityType::CommandCore},
        {"hc_interval_loom", Faction::HollowChoir, EntityType::Dropoff},
        {"hc_chorus_loom", Faction::HollowChoir, EntityType::Barracks},
        {"hc_phase_anchor", Faction::HollowChoir, EntityType::UtilityStructure},
    }};
    const std::string buildingText =
        ReadSourceFile("Content/Data/Source/buildings.json");
    std::int32_t boundBuildings = 0;
    for (const std::string& record : SplitRecords(buildingText, "buildings")) {
        const std::string id = RecordId(record);
        const Binding* binding = nullptr;
        for (const Binding& candidate : buildingBindings) {
            if (id == candidate.id) {
                binding = &candidate;
                break;
            }
        }
        if (binding == nullptr) {
            continue;
        }
        EntityArchetypeRules& archetype =
            load.rules.archetypes[static_cast<std::size_t>(binding->faction)]
                                 [static_cast<std::size_t>(binding->type)];
        archetype.cost = {
            static_cast<std::int32_t>(RequireInt(record, "matter", id)),
            static_cast<std::int32_t>(RequireInt(record, "dawn", id))};
        archetype.maxHitPoints =
            static_cast<std::int32_t>(RequireInt(record, "max_health", id));
        archetype.visionTiles = static_cast<std::int32_t>(
            (RequireInt(record, "sight_cm", id) + 99) / 100);
        archetype.constructionRequired = static_cast<std::int32_t>(
            RequireInt(record, "construction_ticks", id));
        if (HasKey(record, "logistics_capacity")) {
            archetype.populationCapacity = static_cast<std::int32_t>(
                RequireInt(record, "logistics_capacity", id));
        }
        // footprint_cells is the full span; the simulation stores a half extent.
        const std::int64_t cells = RequireInt(record, "footprint_cells", id);
        archetype.footprintHalfExtentRaw =
            static_cast<std::int32_t>(cells * kFixedScale / 2);
        ++boundBuildings;
    }
    if (boundBuildings != static_cast<std::int32_t>(buildingBindings.size())) {
        ContentRuleFailure("bound " + std::to_string(boundBuildings) +
                           " authored buildings, expected 12");
    }

    // Order-stable digest of every archetype field that reached the simulation,
    // so a retained result names the exact ruleset it measured.
    std::uint64_t digest = 0xcbf29ce484222325ULL;
    const auto mix = [&digest](std::int64_t value) {
        digest ^= static_cast<std::uint64_t>(value);
        digest *= 0x100000001b3ULL;
    };
    for (std::size_t faction = 0; faction < load.rules.archetypes.size();
         ++faction) {
        for (std::size_t type = 0;
             type < load.rules.archetypes[faction].size(); ++type) {
            const EntityArchetypeRules& a =
                load.rules.archetypes[faction][type];
            mix(static_cast<std::int64_t>(faction));
            mix(static_cast<std::int64_t>(type));
            mix(a.cost.material);
            mix(a.cost.dawnshards);
            mix(a.maxHitPoints);
            mix(a.movementPerTickRaw);
            mix(a.visionTiles);
            mix(a.attackRangeRaw);
            mix(a.attackDamage);
            mix(static_cast<std::int64_t>(a.attackPeriodTicks));
            mix(a.workRate);
            mix(a.cargoCapacity);
            mix(a.constructionRequired);
            mix(a.populationCost);
            mix(a.populationCapacity);
            mix(a.productionTicks);
            mix(a.footprintHalfExtentRaw);
        }
    }
    load.archetypeChecksum = digest;

    if (load.lancerMaxHealth != 145 || load.lancerCooldownTicks != 30) {
        ContentRuleFailure(
            "authored Meridian Lancer reads " +
            std::to_string(load.lancerMaxHealth) + " health / " +
            std::to_string(static_cast<std::uint64_t>(load.lancerCooldownTicks)) +
            " tick cooldown; the shipped catalog authors 145 / 30");
    }
    return load;
}

const ContentRuleLoad& AuthoredRules() {
    static const ContentRuleLoad loaded = LoadAuthoredRules(20);
    return loaded;
}

// Map identity for a measured match. `TournamentSymmetric` is the legacy
// synthetic fixture this harness has always used; the other three are the
// shipping battlefields of SPEC-SKM-011..013, read from
// EchoesSimCore/SkirmishMapPresets.h so there is no second copy to drift.
//
// The harness could not reach the shipping maps before, because their geometry
// lived in the Unreal module. It reported that itself, in every matrix it ever
// wrote: "synthetic single-map fixture is not the three shipping maps". Balance
// numbers taken on a map the game does not ship cannot discharge SPEC-BAL-003,
// whose own VERIF clause asks for the shipping set.
enum class MapLayout {
    TournamentSymmetric,
    GlassScar,
    CrownfallBasin,
    SorynConfluence,
};

const char* MapLayoutId(MapLayout layout) {
    switch (layout) {
        case MapLayout::GlassScar: return "GlassScar";
        case MapLayout::CrownfallBasin: return "CrownfallBasin";
        case MapLayout::SorynConfluence: return "SorynConfluence";
        case MapLayout::TournamentSymmetric: break;
    }
    return "TournamentSymmetric64";
}

[[nodiscard]] bool IsShippingMap(MapLayout layout) {
    return layout != MapLayout::TournamentSymmetric;
}

[[nodiscard]] SkirmishMapPreset ToPreset(MapLayout layout) {
    switch (layout) {
        case MapLayout::CrownfallBasin: return SkirmishMapPreset::CrownfallBasin;
        case MapLayout::SorynConfluence: return SkirmishMapPreset::SorynConfluence;
        case MapLayout::GlassScar:
        case MapLayout::TournamentSymmetric: break;
    }
    return SkirmishMapPreset::GlassScar;
}

struct MatchRecord {
    std::uint64_t seed = 0;
    std::string mapId = "TournamentSymmetric64";
    std::string faction0;
    std::string faction1;
    std::int32_t slot0 = 0;
    std::int32_t slot1 = 1;
    std::string personality0;
    std::string personality1;
    std::string winnerFaction;
    std::int32_t winnerPlayer = -2;  // 0, 1, -1 (authoritative Draw), -2 (unresolved)
    Tick durationTicks = 0;
    std::uint64_t finalChecksum = 0;
    MatchOutcome outcome = MatchOutcome::Ongoing;
    bool terminal = false;
    Tick lastMaterialProgressTick = 0;
    std::int32_t player0CoreHitPoints = 0;
    std::int32_t player1CoreHitPoints = 0;
    std::string termination = "tick_budget_actionable_stall";
    std::string stallReason;
    // Which seat's commands were queued first this match. Seat 0 was always
    // first, and in a symmetric race that alone decided the winner, so the
    // order is now varied and REPORTED as a dimension rather than averaged
    // away: it is a finding about the game, not noise.
    bool seat1PlannedFirst = false;
    // The rerun check must reproduce the sampled match, not a default one.
    // Personalities were stored only as display strings, and with every match
    // Adaptive that was harmless; conditions now vary personality and planning
    // order, so a rerun that assumes Adaptive/seat-0-first replays a DIFFERENT
    // condition and reports a determinism violation that is not one.
    AiPersonality personality0Enum = AiPersonality::Adaptive;
    AiPersonality personality1Enum = AiPersonality::Adaptive;
    // Conditions are the unit of sampling. Matches sharing a condition differ
    // only by seed, so a condition whose finishing ticks are all identical is
    // one match replayed and carries the information of one observation.
    std::string conditionId;
    std::string mapLayoutId;
    // The rerun check must reproduce the sampled match, not a default one.
    // The harness already learned this for personalities: a rerun that assumed
    // Adaptive/seat-0-first replayed a DIFFERENT condition and reported a
    // determinism violation that was not one. Adding the map as a dimension
    // reopened exactly that hole in a new place - the first grid run over the
    // shipping maps reported a false violation because the rerun replayed the
    // seed on the synthetic fixture. Same lesson, new axis: every dimension a
    // condition varies must travel with the record.
    MapLayout layout = MapLayout::TournamentSymmetric;
    // REL-AI-024. Counted apart on purpose: "never reachable" and "reachable
    // and declined" are indistinguishable in outcomes, and reading an
    // unchanged matrix as a clean null is the error this measurement exists
    // to avoid.
    std::int64_t denialOpportunityTicks = 0;
    std::int64_t denialAchievedTicks = 0;
};

struct MaterialProgressState {
    std::array<std::int64_t, 2> coreHitPoints{};
    std::array<std::int64_t, 2> ownedEntityCount{};
    std::array<std::int64_t, 2> ownedHitPoints{};
    std::array<std::int64_t, 2> resources{};
    std::int64_t resourceRemaining = 0;
    std::int64_t wellLifecycle = 0;

    friend bool operator==(const MaterialProgressState&,
                           const MaterialProgressState&) = default;
};

MaterialProgressState CaptureMaterialProgress(const Simulation& sim) {
    MaterialProgressState state{};
    for (PlayerId player = 0; player < 2; ++player) {
        if (const PlayerState* p = sim.FindPlayer(player)) {
            state.resources[player] =
                static_cast<std::int64_t>(p->resources.material) * 4096 +
                p->resources.dawnshards;
        }
    }
    for (const Entity& entity : sim.Entities()) {
        if (entity.type == EntityType::ResourceNode) {
            state.resourceRemaining += entity.resourceRemaining;
            continue;
        }
        if (entity.type == EntityType::FutureWell) {
            state.wellLifecycle +=
                static_cast<std::int64_t>(entity.owner) * 1000000000LL +
                static_cast<std::int64_t>(entity.wellChoice) * 1000000LL +
                static_cast<std::int64_t>(entity.wellPendingChoice) * 10000LL +
                entity.wellCaptureProgress * 10LL +
                static_cast<std::int64_t>(entity.wellProtocolTicks > 0);
            continue;
        }
        if (entity.owner < 2) {
            ++state.ownedEntityCount[entity.owner];
            state.ownedHitPoints[entity.owner] += entity.hitPoints;
            if (entity.type == EntityType::CommandCore) {
                state.coreHitPoints[entity.owner] += entity.hitPoints;
            }
        }
    }
    return state;
}

std::string DiagnoseStall(const Simulation& sim,
                          AiPersonality p0,
                          AiPersonality p1) {
    std::array<std::size_t, 2> generated{};
    std::array<std::int32_t, 2> workers{};
    std::array<std::int32_t, 2> combat{};
    std::array<std::int32_t, 2> producers{};
    for (PlayerId player = 0; player < 2; ++player) {
        const auto view = sim.CreatePlayerView(player);
        if (view.has_value()) {
            generated[player] = Simulation::GenerateAiCommands(
                *view, player == 0 ? p0 : p1).size();
        }
    }
    for (const Entity& entity : sim.Entities()) {
        if (entity.owner >= 2 || entity.hitPoints <= 0) continue;
        workers[entity.owner] += entity.type == EntityType::Worker ? 1 : 0;
        combat[entity.owner] +=
            entity.type == EntityType::Soldier ||
                    entity.type == EntityType::HeavyUnit ||
                    entity.type == EntityType::ScoutUnit
                ? 1
                : 0;
        producers[entity.owner] +=
            entity.type == EntityType::CommandCore ||
                    entity.type == EntityType::Barracks
                ? 1
                : 0;
    }
    std::ostringstream out;
    out << "ongoing_at_tick_budget"
        << "; generated=" << generated[0] << "/" << generated[1]
        << "; pending=" << sim.PendingCommands().size()
        << "; workers=" << workers[0] << "/" << workers[1]
        << "; combat=" << combat[0] << "/" << combat[1]
        << "; producers=" << producers[0] << "/" << producers[1];
    if (generated[0] == 0 || generated[1] == 0) {
        out << "; action=no_commands_for_live_seat";
    } else {
        out << "; action=commands_fail_to_convert_into_corefall";
    }
    return out.str();
}

inline std::string FactionToString(Faction f) {
    switch (f) {
        case Faction::MeridianCompact:
            return "MeridianCompact";
        case Faction::KharuunAssemblies:
            return "KharuunAssemblies";
        case Faction::HollowChoir:
            return "HollowChoir";
        default:
            return "Unknown";
    }
}

inline std::string PersonalityToString(AiPersonality p) {
    switch (p) {
        case AiPersonality::Balanced:
            return "Balanced";
        case AiPersonality::Defensive:
            return "Defensive";
        case AiPersonality::Raider:
            return "Raider";
        case AiPersonality::Economic:
            return "Economic";
        case AiPersonality::Expansionist:
            return "Expansionist";
        case AiPersonality::Adaptive:
            return "Adaptive";
        default:
            return "Unknown";
    }
}

void SetupTournamentMap(Simulation& sim, Faction f0, Faction f1) {
    sim.AddPlayer(0, f0, ResourcePool{800, 350});
    sim.AddPlayer(1, f1, ResourcePool{800, 350});

    // Openings are mirrored across the 64x64 diagonal. Workers stand clear of
    // the headquarters footprint: an authored HQ is five cells across, so the
    // old spawn ring sat inside the building's own ground.
    sim.SpawnEntity(0, f0, EntityType::CommandCore, Vec2::FromTiles(10, 10));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(14, 10));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(10, 14));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(14, 14));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(6, 14));
    sim.SpawnResourceNode(Vec2::FromTiles(5, 10), 10000);
    sim.SpawnFutureWell(Vec2::FromTiles(10, 5));

    sim.SpawnEntity(1, f1, EntityType::CommandCore, Vec2::FromTiles(54, 54));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(50, 54));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(54, 50));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(50, 50));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(58, 50));
    sim.SpawnResourceNode(Vec2::FromTiles(59, 54), 10000);
    sim.SpawnFutureWell(Vec2::FromTiles(54, 59));

    // Contested neutral centre — resources only, no well (wells are per-base)
    sim.SpawnResourceNode(Vec2::FromTiles(28, 32), 8000);
    sim.SpawnResourceNode(Vec2::FromTiles(36, 32), 8000);
    sim.SpawnResourceNode(Vec2::FromTiles(32, 28), 6000);
    sim.SpawnResourceNode(Vec2::FromTiles(32, 36), 6000);
}

// One shipping battlefield, assembled from the shared preset tables: blocked
// ground, both forces in spawn-tile order, the authored deposits and the Well.
// Nothing here is transcribed - every figure is read from
// SkirmishMapPresets.h, which the Unreal skirmish model now also reads, so the
// harness and the game cannot disagree about what a map is.
void SetupShippingMap(Simulation& sim,
                      MapLayout layout,
                      Faction f0,
                      Faction f1) {
    const SkirmishMapPreset preset = ToPreset(layout);

    // Terrain first: a force spawned before the ground exists can land inside
    // a ridge. The synthetic fixture has no blocked ground at all, which is a
    // second reason its numbers never described a shipping match - routes,
    // gates and chokes are most of what these maps are.
    for (std::int32_t tileY = 0; tileY < kSkirmishMapHeightTiles; ++tileY) {
        for (std::int32_t tileX = 0; tileX < kSkirmishMapWidthTiles; ++tileX) {
            if (IsSkirmishBlockedTile(preset, tileX, tileY)) {
                (void)sim.SetTerrainTile(tileX, tileY, Terrain::Blocked);
            }
        }
    }

    sim.AddPlayer(0, f0, ResourcePool{kSkirmishStandardMaterial,
                                      kSkirmishStandardDawn});
    sim.AddPlayer(1, f1, ResourcePool{kSkirmishStandardMaterial,
                                      kSkirmishStandardDawn});

    const auto localTiles = SkirmishLocalSpawnTiles(preset);
    for (std::size_t index = 0; index < localTiles.size(); ++index) {
        sim.SpawnEntity(0, f0, kSkirmishLocalForce[index],
                        Vec2::FromTiles(localTiles[index].x,
                                        localTiles[index].y));
    }
    const auto opponentTiles = SkirmishOpponentSpawnTiles(preset);
    for (std::size_t index = 0; index < opponentTiles.size(); ++index) {
        sim.SpawnEntity(1, f1, kSkirmishOpponentForce[index],
                        Vec2::FromTiles(opponentTiles[index].x,
                                        opponentTiles[index].y));
    }

    for (const SkirmishTile& deposit : SkirmishResourceNodeTiles(preset)) {
        sim.SpawnResourceNode(Vec2::FromTiles(deposit.x, deposit.y),
                              kSkirmishDepositAmount);
    }
    const SkirmishTile well = SkirmishFutureWellTile(preset);
    sim.SpawnFutureWell(Vec2::FromTiles(well.x, well.y));
}

// REL-AI-024 measurement, taken from simulation state rather than from planner
// counters. Two separate facts, because the earlier attempt could not tell them
// apart: whether the OPPORTUNITY to deny ever arises on this map, and whether
// the planner CONVERTS it. A rule that is never reachable and a rule that is
// reachable and declines produce the same match outcome, so they are counted
// apart here.
struct DenialObservation {
    std::int64_t opportunityTicks = 0;  // seat has no Well, foe's Well pays
    std::int64_t achievedTicks = 0;     // ...and one of our bodies stands in it
};

DenialObservation ObserveDenial(const Simulation& sim, PlayerId seat) {
    DenialObservation observation{};
    const PlayerId foe = seat == 0 ? 1 : 0;
    bool seatHoldsWell = false;
    const Entity* foeWell = nullptr;
    for (const Entity& entity : sim.Entities()) {
        if (entity.type != EntityType::FutureWell || entity.hitPoints <= 0 ||
            entity.wellChoice != FutureWellChoice::Preserve) {
            continue;
        }
        if (entity.owner == seat) {
            seatHoldsWell = true;
        } else if (entity.owner == foe && entity.wellActivationTick != 0) {
            foeWell = &entity;
        }
    }
    if (seatHoldsWell || foeWell == nullptr) {
        return observation;
    }
    observation.opportunityTicks = 1;
    const std::int64_t radius = sim.FutureWellCaptureRadiusRaw();
    const std::int64_t zone = radius * radius;
    for (const Entity& unit : sim.Entities()) {
        if (unit.owner != seat || unit.hitPoints <= 0) {
            continue;
        }
        const std::int64_t dx = static_cast<std::int64_t>(unit.position.x.Raw()) -
                                foeWell->position.x.Raw();
        const std::int64_t dy = static_cast<std::int64_t>(unit.position.y.Raw()) -
                                foeWell->position.y.Raw();
        if (dx * dx + dy * dy <= zone) {
            observation.achievedTicks = 1;
            break;
        }
    }
    return observation;
}

MatchRecord RunMatch(std::uint64_t seed,
                     Faction f0,
                     Faction f1,
                     AiPersonality p0,
                     AiPersonality p1,
                     Tick maxTicks = 12000,
                     bool seat1PlansFirst = false,
                     MapLayout layout = MapLayout::TournamentSymmetric) {
    // 64x64 matches every shipped preset; the synthetic 48x48 matched none.
    SimulationConfig config{64, 64, 20, seed};
    config.rules = AuthoredRules().rules;
    Simulation sim(config);
    if (IsShippingMap(layout)) {
        SetupShippingMap(sim, layout, f0, f1);
    } else {
        SetupTournamentMap(sim, f0, f1);
    }

    MatchRecord record{};
    record.mapId = MapLayoutId(layout);
    record.mapLayoutId = MapLayoutId(layout);
    record.layout = layout;
    record.seed = seed;
    record.seat1PlannedFirst = seat1PlansFirst;
    record.faction0 = FactionToString(f0);
    record.faction1 = FactionToString(f1);
    record.personality0 = PersonalityToString(p0);
    record.personality1 = PersonalityToString(p1);
    record.personality0Enum = p0;
    record.personality1Enum = p1;

    Tick ticks = 0;
    MaterialProgressState priorProgress = CaptureMaterialProgress(sim);
    Tick lastMaterialProgressTick = 0;
    while (sim.Outcome() == MatchOutcome::Ongoing && ticks < maxTicks) {
        if (ticks % 4 == 0) {
            // Queue order is the only asymmetry in a mirror, so it decides the
            // race. Alternating it does not remove that; it exposes it.
            const PlayerId firstSeat = seat1PlansFirst ? 1 : 0;
            const PlayerId secondSeat = seat1PlansFirst ? 0 : 1;
            for (const auto& c : sim.GenerateAiCommands(
                     firstSeat, firstSeat == 0 ? p0 : p1)) {
                sim.QueueCommand(c);
            }
            for (const auto& c : sim.GenerateAiCommands(
                     secondSeat, secondSeat == 0 ? p0 : p1)) {
                sim.QueueCommand(c);
            }
        }
        sim.Step();
        ++ticks;
        if (ticks % sim.Config().ticksPerSecond == 0) {
            for (PlayerId seat = 0; seat < 2; ++seat) {
                const DenialObservation observed = ObserveDenial(sim, seat);
                record.denialOpportunityTicks += observed.opportunityTicks;
                record.denialAchievedTicks += observed.achievedTicks;
            }
            const MaterialProgressState current = CaptureMaterialProgress(sim);
            if (!(current == priorProgress)) {
                lastMaterialProgressTick = ticks;
                priorProgress = current;
            }
        }
    }

    record.durationTicks = ticks;
    record.finalChecksum = sim.StateChecksum();

    const MatchOutcome outcome = sim.Outcome();
    record.outcome = outcome;
    record.terminal = outcome != MatchOutcome::Ongoing;
    record.lastMaterialProgressTick = lastMaterialProgressTick;
    const MaterialProgressState finalProgress = CaptureMaterialProgress(sim);
    record.player0CoreHitPoints =
        static_cast<std::int32_t>(finalProgress.coreHitPoints[0]);
    record.player1CoreHitPoints =
        static_cast<std::int32_t>(finalProgress.coreHitPoints[1]);
    if (outcome == MatchOutcome::Player0Victory) {
        record.winnerPlayer = 0;
        record.winnerFaction = record.faction0;
        record.termination = "authoritative_corefall";
    } else if (outcome == MatchOutcome::Player1Victory) {
        record.winnerPlayer = 1;
        record.winnerFaction = record.faction1;
        record.termination = "authoritative_corefall";
    } else if (outcome == MatchOutcome::Draw) {
        record.winnerPlayer = -1;
        record.winnerFaction = "Draw";
        record.termination = "authoritative_draw";
    } else {
        record.winnerPlayer = -2;
        record.winnerFaction.clear();
        record.stallReason = DiagnoseStall(sim, p0, p1);
    }

    return record;
}

// 95% Wilson score binomial confidence interval
struct ConfidenceInterval {
    double rate = 0.0;
    double lower = 0.0;
    double upper = 0.0;
    double marginOfError = 0.0;
};

[[nodiscard]] std::string Pct(double fraction) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", fraction * 100.0);
    return std::string(buffer);
}

// A window verdict is only meaningful over a non-empty sample. Rendering an
// empty sample as 0.0% FAIL states a measurement that was never taken, which is
// the same false precision as an interval over replays, pointed the other way.
[[nodiscard]] const char* WindowVerdict(int total, double rate, double low,
                                        double high) {
    if (total <= 0) {
        return "NO SAMPLE";
    }
    return (rate >= low && rate <= high) ? "PASS" : "FAIL";
}

ConfidenceInterval ComputeConfidenceInterval(int successes, int total) {
    if (total <= 0) {
        return {0.0, 0.0, 0.0, 0.0};
    }
    const double p = static_cast<double>(successes) / static_cast<double>(total);
    constexpr double z = 1.95996;  // 95% confidence level
    const double z2 = z * z;
    const double denominator = 1.0 + z2 / total;
    const double center = (p + z2 / (2.0 * total)) / denominator;
    const double halfWidth = (z * std::sqrt((p * (1.0 - p) / total) + (z2 / (4.0 * total * total)))) / denominator;

    ConfidenceInterval ci;
    ci.rate = p;
    ci.lower = std::max(0.0, center - halfWidth);
    ci.upper = std::min(1.0, center + halfWidth);
    ci.marginOfError = halfWidth;
    return ci;
}

}  // namespace echoes::balance

int main(int argc, char* argv[]) {
    using namespace echoes::sim;
    using namespace echoes::balance;

    int totalMatches = 1000;
    std::string outputPath = "balance_matrix_report.json";
    bool runPrimacy = true;
    bool runDeterminism = true;
    bool runBattery = true;
    std::uint64_t baseSeed = 0x8A1A2C3D4E5FULL;
    int requestedThreads = static_cast<int>(std::thread::hardware_concurrency());
    if (requestedThreads <= 0) requestedThreads = 4;
    // SPEC-BAL-003's verification asks for the shipping set, so that is the
    // default. The synthetic fixture stays reachable for continuity with the
    // matrices recorded before the shipping maps were usable here.
    std::vector<MapLayout> layouts = {MapLayout::GlassScar,
                                      MapLayout::CrownfallBasin,
                                      MapLayout::SorynConfluence};

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        if (arg == "--matches" && i + 1 < argc) {
            totalMatches = std::max(9, std::stoi(argv[++i]));
        } else if (arg == "--output" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--seed" && i + 1 < argc) {
            baseSeed = std::stoull(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            requestedThreads = std::max(1, std::stoi(argv[++i]));
        } else if (arg == "--map" && i + 1 < argc) {
            const std::string_view requested(argv[++i]);
            if (requested == "shipping") {
                layouts = {MapLayout::GlassScar, MapLayout::CrownfallBasin,
                           MapLayout::SorynConfluence};
            } else if (requested == "glass-scar") {
                layouts = {MapLayout::GlassScar};
            } else if (requested == "crownfall") {
                layouts = {MapLayout::CrownfallBasin};
            } else if (requested == "soryn") {
                layouts = {MapLayout::SorynConfluence};
            } else if (requested == "tournament") {
                layouts = {MapLayout::TournamentSymmetric};
            } else {
                std::cerr << "Unknown --map " << requested
                          << " (expected shipping, glass-scar, crownfall, "
                             "soryn or tournament)\n";
                return 2;
            }
        }
    }

    std::cout << "========================================================\n";
    std::cout << "Echoes of the Broken Sun — Headless AI Balance Harness\n";
    std::cout << "SPEC-BAL-001..008 Automated 1,000-Match Validation Matrix\n";
    std::cout << "========================================================\n";
    std::cout << "Target Matches: " << totalMatches << " | Threads: " << requestedThreads
              << " | Maps: ";
    for (std::size_t index = 0; index < layouts.size(); ++index) {
        std::cout << (index == 0 ? "" : ", ") << MapLayoutId(layouts[index]);
    }
    std::cout << "\n";

    const auto startTime = std::chrono::high_resolution_clock::now();

    constexpr std::array<Faction, 3> kFactions = {
        Faction::MeridianCompact,
        Faction::KharuunAssemblies,
        Faction::HollowChoir,
    };

    struct MatchTask {
        std::uint64_t seed;
        Faction f0;
        Faction f1;
        AiPersonality p0;
        AiPersonality p1;
        bool seat1PlansFirst;
        MapLayout layout;
        std::string conditionId;
    };

    // Every match ran Adaptive against Adaptive with seat 0 always planning
    // first, so 1,000 matches sampled ONE condition per faction pair. Four of
    // the nine pairings then returned a single finishing tick across 111 seeds
    // (evidence authored-rules-degeneracy-20260912T002357Z): the seed reaches
    // play only through the wander fallback, which a tasked planner never
    // reaches, so those rows were one match replayed. Conditions are varied
    // deliberately instead of injecting noise, which would only manufacture
    // the appearance of diversity.
    //
    // Raider earns its place: it is the only personality that commits Reshape,
    // so without it no seat ever exercises the Well's third protocol or the
    // simulation's only RNG consumer.
    struct PersonalityPair {
        AiPersonality p0;
        AiPersonality p1;
    };
    static const std::array<PersonalityPair, 4> kPersonalityPairs = {{
        {AiPersonality::Adaptive, AiPersonality::Adaptive},
        {AiPersonality::Balanced, AiPersonality::Balanced},
        {AiPersonality::Raider, AiPersonality::Defensive},
        {AiPersonality::Economic, AiPersonality::Expansionist},
    }};

    std::vector<MatchTask> tasks;
    tasks.reserve(totalMatches);

    // The map is a condition dimension, not a run-level setting. SPEC-BAL-003
    // is a claim about every shipping battlefield, and a faction can sit inside
    // the band on one map while failing on another; averaging the three would
    // hide exactly that.
    const int conditionCount = 9 * static_cast<int>(kPersonalityPairs.size()) *
                               2 * static_cast<int>(layouts.size());
    const int seedsPerCondition = std::max(1, totalMatches / conditionCount);

    std::uint64_t seedCursor = 0;
    for (const MapLayout layout : layouts) {
        for (int matchup = 0; matchup < 9; ++matchup) {
            const Faction f0 = kFactions[matchup / 3];
            const Faction f1 = kFactions[matchup % 3];
            for (const PersonalityPair& pair : kPersonalityPairs) {
                for (int order = 0; order < 2; ++order) {
                    const bool seat1First = order == 1;
                    std::string conditionId =
                        std::string(MapLayoutId(layout)) + "|" +
                        FactionToString(f0) + "-v-" + FactionToString(f1) + "|" +
                        PersonalityToString(pair.p0) + "-v-" +
                        PersonalityToString(pair.p1) + "|" +
                        (seat1First ? "seat1-first" : "seat0-first");
                    for (int s = 0; s < seedsPerCondition; ++s) {
                        tasks.push_back({
                            baseSeed + (seedCursor++) * 10007ULL,
                            f0, f1, pair.p0, pair.p1, seat1First, layout,
                            conditionId});
                    }
                }
            }
        }
    }
    totalMatches = static_cast<int>(tasks.size());
    std::cout << "Conditions: " << conditionCount << " | seeds per condition: "
              << seedsPerCondition << " | matches: " << totalMatches << "\n";

    std::vector<MatchRecord> results(totalMatches);
    std::mutex progressMutex;
    std::atomic<int> completedTasks{0};

    auto worker = [&](int threadId) {
        for (int idx = threadId; idx < totalMatches; idx += requestedThreads) {
            const auto& task = tasks[idx];
            MatchRecord rec = RunMatch(task.seed, task.f0, task.f1, task.p0,
                                       task.p1, 12000, task.seat1PlansFirst,
                                       task.layout);
            rec.conditionId = task.conditionId;
            results[idx] = rec;
            const int finished = ++completedTasks;
            if (finished % 100 == 0 || finished == totalMatches) {
                std::lock_guard<std::mutex> lock(progressMutex);
                std::cout << "Progress: " << finished << "/" << totalMatches
                          << " matches (" << (finished * 100 / totalMatches) << "%)\n";
            }
        }
    };

    std::vector<std::thread> threads;
    for (int t = 0; t < requestedThreads; ++t) {
        threads.emplace_back(worker, t);
    }
    for (auto& th : threads) {
        th.join();
    }

    const auto endTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> duration = endTime - startTime;
    const double elapsedSec = duration.count();
    const double matchesPerSec = totalMatches / std::max(0.001, elapsedSec);

    std::cout << "\nBatch simulation completed in " << std::fixed << std::setprecision(2)
              << elapsedSec << "s (" << matchesPerSec << " matches/sec)\n";

    // REL-AI-024 aggregation. Kept beside the batch it summarises so the two
    // counts cannot be read as one.
    std::int64_t denialOpportunitySamples = 0;
    std::int64_t denialAchievedSamples = 0;
    int denialOpportunityMatches = 0;
    int denialAchievedMatches = 0;
    for (const MatchRecord& r : results) {
        denialOpportunitySamples += r.denialOpportunityTicks;
        denialAchievedSamples += r.denialAchievedTicks;
        denialOpportunityMatches += r.denialOpportunityTicks > 0 ? 1 : 0;
        denialAchievedMatches += r.denialAchievedTicks > 0 ? 1 : 0;
    }
    std::cout << "REL-AI-024 denial: opportunity in " << denialOpportunityMatches
              << "/" << totalMatches << " matches (" << denialOpportunitySamples
              << " samples); achieved in " << denialAchievedMatches << " matches ("
              << denialAchievedSamples << " samples)\n";

    // SPEC-BAL-003 / REL-AI-042 measurement grid: one cell per faction pairing
    // per shipping map, which is the claim those requirements actually make.
    // A faction can sit inside the 40-60% band on one battlefield and fail on
    // another, so the three maps are never averaged together.
    //
    // Degenerate conditions are excluded here for the same reason they are
    // excluded from every other rate in this report: a condition whose matches
    // all finish on the same tick is one observation replayed, and counting its
    // 13 copies as 13 wins manufactures confidence that does not exist. The
    // excluded totals are reported beside each cell rather than dropped
    // silently, so a cell that is mostly replays is visible as such.
    struct BalanceCell {
        std::string map;
        std::string pairing;
        bool mirror = false;
        int decisive = 0;      // counted, non-degenerate, a side won
        int faction0Wins = 0;
        int draws = 0;
        int unresolved = 0;
        int excludedDegenerate = 0;
        int seat0Wins = 0;
        int seat1Wins = 0;
    };
    std::map<std::string, BalanceCell> balanceCells;
    // Built after `conditions` so degeneracy is known; see below.
    // 0. Condition summary, and the degeneracy screen that must run FIRST.
    // A condition is a faction pair, a personality pair and a planning order;
    // its matches differ only by seed. If every match in a condition finishes
    // on the same tick, the seed changed nothing and the condition is one
    // observation replayed, whatever its match count says. Statistics computed
    // over such rows are not measurements, and the previous harness had no way
    // to notice: it reported Wilson intervals over 111 replays.
    struct ConditionSummary {
        std::string id;
        int matches = 0;
        int seat0Wins = 0;
        int seat1Wins = 0;
        int draws = 0;
        int unresolved = 0;
        std::set<Tick> distinctTicks;
        [[nodiscard]] bool Degenerate() const {
            return matches > 1 && distinctTicks.size() == 1;
        }
    };
    std::map<std::string, ConditionSummary> conditions;
    for (const auto& r : results) {
        ConditionSummary& c = conditions[r.conditionId];
        c.id = r.conditionId;
        ++c.matches;
        c.distinctTicks.insert(r.durationTicks);
        if (r.winnerPlayer == 0) ++c.seat0Wins;
        else if (r.winnerPlayer == 1) ++c.seat1Wins;
        else if (r.winnerPlayer == -1) ++c.draws;
        else ++c.unresolved;
    }
    int degenerateConditions = 0;
    int degenerateMatches = 0;
    for (const auto& [id, c] : conditions) {
        if (c.Degenerate()) {
            ++degenerateConditions;
            degenerateMatches += c.matches;
        }
    }
    const auto Sampled = [&conditions](const MatchRecord& r) {
        const auto it = conditions.find(r.conditionId);
        return it != conditions.end() && !it->second.Degenerate();
    };
    std::cout << "\nConditions: " << conditions.size() << " | degenerate: "
              << degenerateConditions << " (" << degenerateMatches
              << " matches carrying " << degenerateConditions
              << " observations)\n";
    if (degenerateConditions > 0) {
        std::cout << "Degenerate conditions are excluded from every rate and "
                     "interval below.\n";
    }

    // Populate the SPEC-BAL-003 grid now that degeneracy is known.
    for (const MatchRecord& r : results) {
        const std::string key = r.mapLayoutId + "|" + r.faction0 + "-v-" + r.faction1;
        BalanceCell& cell = balanceCells[key];
        cell.map = r.mapLayoutId;
        cell.pairing = r.faction0 + "-v-" + r.faction1;
        cell.mirror = r.faction0 == r.faction1;
        if (!Sampled(r)) {
            ++cell.excludedDegenerate;
            continue;
        }
        if (r.winnerPlayer == 0) {
            ++cell.decisive; ++cell.faction0Wins; ++cell.seat0Wins;
        } else if (r.winnerPlayer == 1) {
            ++cell.decisive; ++cell.seat1Wins;
        } else if (r.winnerPlayer == -1) {
            ++cell.draws;
        } else {
            ++cell.unresolved;
        }
    }

    // SPEC-BAL-003 is a claim about NON-MIRROR pairings; a mirror is 50% by
    // construction and is reported only as a control that the instrument is
    // not seat-biased into nonsense.
    int cellsInBand = 0;
    int cellsOutOfBand = 0;
    int cellsUnderpowered = 0;
    for (const auto& [key, cell] : balanceCells) {
        if (cell.mirror) continue;
        if (cell.decisive < 30) { ++cellsUnderpowered; continue; }
        const double rate = static_cast<double>(cell.faction0Wins) / cell.decisive;
        if (rate < 0.40 || rate > 0.60) ++cellsOutOfBand; else ++cellsInBand;
    }
    std::cout << "\nSPEC-BAL-003 grid (faction pairing x shipping map), "
                 "degenerate conditions excluded:\n";
    for (const auto& [key, cell] : balanceCells) {
        if (cell.mirror) continue;
        std::cout << "  " << std::left << std::setw(52) << key << std::right;
        if (cell.decisive < 30) {
            std::cout << "  decisive=" << std::setw(4) << cell.decisive
                      << "  (too few to judge; " << cell.excludedDegenerate
                      << " excluded as replays)\n";
            continue;
        }
        const double rate =
            static_cast<double>(cell.faction0Wins) / cell.decisive;
        std::cout << "  decisive=" << std::setw(4) << cell.decisive
                  << "  rate=" << std::fixed << std::setprecision(3) << rate
                  << (rate < 0.40 || rate > 0.60 ? "  OUT OF BAND" : "  in band")
                  << "  (excluded replays " << cell.excludedDegenerate << ")\n";
    }
    std::cout << "  cells in band: " << cellsInBand << " | out of band: "
              << cellsOutOfBand << " | too few decisive matches: "
              << cellsUnderpowered << "\n";

    // 1. Evaluate Spawn Slot Fairness (SPEC-BAL-004)
    int slot0Wins = 0;
    int slot1Wins = 0;
    int draws = 0;
    int unresolved = 0;
    for (const auto& r : results) {
        if (!Sampled(r)) continue;
        if (r.winnerPlayer == 0) slot0Wins++;
        else if (r.winnerPlayer == 1) slot1Wins++;
        else if (r.winnerPlayer == -1) draws++;
        else unresolved++;
    }
    const int decisiveMatches = slot0Wins + slot1Wins;
    const auto spawnCI = ComputeConfidenceInterval(slot0Wins, decisiveMatches);
    const bool spawnFairnessPassed =
        decisiveMatches > 0 && spawnCI.rate >= 0.45 && spawnCI.rate <= 0.55;

    // 2. Evaluate Non-Mirror Pairings Balance Band (SPEC-BAL-003)
    struct PairStats {
        int winsA = 0;
        int winsB = 0;
        int pairDraws = 0;
    };
    std::array<PairStats, 3> nonMirrorPairs; // 0: M vs K, 1: M vs C, 2: K vs C

    for (const auto& r : results) {
        if (!Sampled(r)) continue;
        if (r.faction0 == "MeridianCompact" && r.faction1 == "KharuunAssemblies") {
            if (r.winnerPlayer == 0) nonMirrorPairs[0].winsA++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[0].winsB++;
            else if (r.winnerPlayer == -1) nonMirrorPairs[0].pairDraws++;
        } else if (r.faction0 == "KharuunAssemblies" && r.faction1 == "MeridianCompact") {
            if (r.winnerPlayer == 0) nonMirrorPairs[0].winsB++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[0].winsA++;
            else if (r.winnerPlayer == -1) nonMirrorPairs[0].pairDraws++;
        } else if (r.faction0 == "MeridianCompact" && r.faction1 == "HollowChoir") {
            if (r.winnerPlayer == 0) nonMirrorPairs[1].winsA++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[1].winsB++;
            else if (r.winnerPlayer == -1) nonMirrorPairs[1].pairDraws++;
        } else if (r.faction0 == "HollowChoir" && r.faction1 == "MeridianCompact") {
            if (r.winnerPlayer == 0) nonMirrorPairs[1].winsB++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[1].winsA++;
            else if (r.winnerPlayer == -1) nonMirrorPairs[1].pairDraws++;
        } else if (r.faction0 == "KharuunAssemblies" && r.faction1 == "HollowChoir") {
            if (r.winnerPlayer == 0) nonMirrorPairs[2].winsA++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[2].winsB++;
            else if (r.winnerPlayer == -1) nonMirrorPairs[2].pairDraws++;
        } else if (r.faction0 == "HollowChoir" && r.faction1 == "KharuunAssemblies") {
            if (r.winnerPlayer == 0) nonMirrorPairs[2].winsB++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[2].winsA++;
            else if (r.winnerPlayer == -1) nonMirrorPairs[2].pairDraws++;
        }
    }

    const auto mkCI = ComputeConfidenceInterval(nonMirrorPairs[0].winsA, nonMirrorPairs[0].winsA + nonMirrorPairs[0].winsB);
    const auto mcCI = ComputeConfidenceInterval(nonMirrorPairs[1].winsA, nonMirrorPairs[1].winsA + nonMirrorPairs[1].winsB);
    const auto kcCI = ComputeConfidenceInterval(nonMirrorPairs[2].winsA, nonMirrorPairs[2].winsA + nonMirrorPairs[2].winsB);

    const auto InBand = [](const ConfidenceInterval& ci, int total) {
        return total > 0 && ci.rate >= 0.40 && ci.rate <= 0.60;
    };
    const int mkTotal = nonMirrorPairs[0].winsA + nonMirrorPairs[0].winsB;
    const int mcTotal = nonMirrorPairs[1].winsA + nonMirrorPairs[1].winsB;
    const int kcTotal = nonMirrorPairs[2].winsA + nonMirrorPairs[2].winsB;
    const bool mkPassed = InBand(mkCI, mkTotal);
    const bool mcPassed = InBand(mcCI, mcTotal);
    const bool kcPassed = InBand(kcCI, kcTotal);
    const bool balanceBandPassed = mkPassed && mcPassed && kcPassed;

    // 3. Strategy Primacy Validation (SPEC-BAL-005)
    int primacyHighWins = 0;
    int primacyFlawedWins = 0;
    if (runPrimacy) {
        std::cout << "\nExecuting Strategy Primacy battery (Adaptive vs Economic)...\n";
        for (int i = 0; i < 50; ++i) {
            const MatchRecord r = RunMatch(0xFEED0000ULL + i * 31,
                                           Faction::MeridianCompact,
                                           Faction::MeridianCompact,
                                           AiPersonality::Adaptive,
                                           AiPersonality::Economic);
            if (r.winnerPlayer == 0) primacyHighWins++;
            else if (r.winnerPlayer == 1) primacyFlawedWins++;
        }
    }
    const auto primacyCI = ComputeConfidenceInterval(primacyHighWins, primacyHighWins + primacyFlawedWins);
    const bool primacyPassed = (primacyCI.rate >= 0.75);

    // 4. Batch Replay Determinism Validation (SPEC-BAL-006)
    bool determinismPassed = true;
    if (runDeterminism) {
        std::cout << "Executing Batch Replay Determinism verification...\n";
        for (int i = 0; i < 10 && determinismPassed; ++i) {
            const auto& sample = results[static_cast<std::size_t>(i) * static_cast<std::size_t>(totalMatches / 10)];
            Faction f0 = Faction::MeridianCompact;
            Faction f1 = Faction::KharuunAssemblies;
            for (auto f : kFactions) {
                if (FactionToString(f) == sample.faction0) f0 = f;
                if (FactionToString(f) == sample.faction1) f1 = f;
            }
            const MatchRecord replay =
                RunMatch(sample.seed, f0, f1, sample.personality0Enum,
                         sample.personality1Enum, 12000,
                         sample.seat1PlannedFirst, sample.layout);
            if (replay.durationTicks != sample.durationTicks ||
                replay.finalChecksum != sample.finalChecksum ||
                replay.winnerPlayer != sample.winnerPlayer ||
                replay.outcome != sample.outcome ||
                replay.termination != sample.termination ||
                replay.stallReason != sample.stallReason) {
                determinismPassed = false;
                std::cerr << "DETERMINISM VIOLATION on seed " << sample.seed << "\n";
            }
        }
    }

    // 5. AI Competence Battery (SPEC-BAL-008)
    bool batteryPassed = true;
    constexpr int implementedBatteryChecks = 1;
    constexpr int requiredBatteryChecks = 4;
    if (runBattery) {
        std::cout << "Executing AI Instrument Competence battery...\n";
        // 1. Retreat severely damaged units
        Simulation retreatSim(SimulationConfig{32, 32, 20, 0x123});
        retreatSim.AddPlayer(0, Faction::MeridianCompact, {1000, 500});
        retreatSim.AddPlayer(1, Faction::MeridianCompact, {1000, 500});
        retreatSim.SpawnEntity(0, Faction::MeridianCompact, EntityType::CommandCore, Vec2::FromTiles(5, 5));
        const EntityId soldier = retreatSim.SpawnEntity(0, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(15, 15));
        const EntityId enemy = retreatSim.SpawnEntity(1, Faction::MeridianCompact, EntityType::Soldier, Vec2::FromTiles(16, 15));
        Command atk{};
        atk.executeTick = 0;
        atk.player = 1;
        atk.sequence = 1;
        atk.type = CommandType::Attack;
        atk.actor = enemy;
        atk.target = soldier;
        retreatSim.QueueCommand(atk);
        retreatSim.Step(49);
        const auto retreatCmds = retreatSim.GenerateAiCommands(0, AiPersonality::Adaptive);
        const bool hasRetreatOrder = std::any_of(retreatCmds.begin(), retreatCmds.end(), [&](const Command& c) {
            return c.actor == soldier && c.type == CommandType::Move;
        });
        if (!hasRetreatOrder) {
            batteryPassed = false;
            std::cerr << "AI Competency Battery Failed: Damaged unit did not retreat!\n";
        }
    }

    std::cout << "\n================ Balance Summary ================\n";
    std::cout << "Spawn Symmetry (Slot 0 win rate): "
              << (decisiveMatches == 0
                      ? std::string("NO SAMPLE (every contributing condition was degenerate)")
                      : Pct(spawnCI.rate) + "% (N=" +
                            std::to_string(decisiveMatches) + ") [" +
                            WindowVerdict(decisiveMatches, spawnCI.rate, 0.45, 0.55) + "]")
              << "\n";
    {
        const int total = nonMirrorPairs[0].winsA + nonMirrorPairs[0].winsB;
        std::cout << "Meridian vs Kharuun:              "
                  << (total == 0
                          ? std::string("NO SAMPLE (all contributing conditions degenerate)")
                          : Pct(mkCI.rate) + "% ± " +
                                Pct(mkCI.marginOfError) + "% (N=" +
                                std::to_string(total) + ") [" +
                                WindowVerdict(total, mkCI.rate, 0.40, 0.60) + "]")
                  << "\n";
    }
    {
        const int total = nonMirrorPairs[1].winsA + nonMirrorPairs[1].winsB;
        std::cout << "Meridian vs Hollow Choir:         "
                  << (total == 0
                          ? std::string("NO SAMPLE (all contributing conditions degenerate)")
                          : Pct(mcCI.rate) + "% ± " +
                                Pct(mcCI.marginOfError) + "% (N=" +
                                std::to_string(total) + ") [" +
                                WindowVerdict(total, mcCI.rate, 0.40, 0.60) + "]")
                  << "\n";
    }
    {
        const int total = nonMirrorPairs[2].winsA + nonMirrorPairs[2].winsB;
        std::cout << "Kharuun vs Hollow Choir:          "
                  << (total == 0
                          ? std::string("NO SAMPLE (all contributing conditions degenerate)")
                          : Pct(kcCI.rate) + "% ± " +
                                Pct(kcCI.marginOfError) + "% (N=" +
                                std::to_string(total) + ") [" +
                                WindowVerdict(total, kcCI.rate, 0.40, 0.60) + "]")
                  << "\n";
    }
    {
        const int primacyTotal = primacyHighWins + primacyFlawedWins;
        std::cout << "Strategy Primacy (Adaptive vs Econ): "
                  << (primacyTotal == 0
                          ? std::string("NO SAMPLE (no primacy match reached a decision)")
                          : Pct(primacyCI.rate) + "% ± " + Pct(primacyCI.marginOfError) +
                                "% (N=" + std::to_string(primacyTotal) + ") [" +
                                (primacyPassed ? "PASS" : "FAIL") + "]")
                  << "\n";
    }
    std::cout << "Duplicate deterministic rerun:    "
              << (determinismPassed
                      ? "10/10 final states matched [PASS]"
                      : "final-state divergence detected [FAIL]")
              << "\n";
    std::cout << "AI Competence Battery:            "
              << implementedBatteryChecks << "/" << requiredBatteryChecks
              << (batteryPassed ? " implemented checks passed [INCOMPLETE]"
                                : " implemented check failed [FAIL]") << "\n";
    std::cout << "Authoritative terminal matches:   "
              << (totalMatches - unresolved) << "/" << totalMatches
              << "; actionable stalls: " << unresolved << "\n";
    std::cout << "=================================================\n";

    // Write structured JSON
    std::ofstream out(outputPath);
    if (out.is_open()) {
        out << "{\n";
        out << "  \"rules_source\": \"" << AuthoredRules().source << "\",\n";
        out << "  \"rules_archetype_checksum\": \""
            << std::hex << std::setw(16) << std::setfill('0')
            << AuthoredRules().archetypeChecksum << std::dec
            << std::setfill(' ') << "\",\n";
        out << "  \"rules_probe\": {\"meridian_lancer_max_health\": "
            << AuthoredRules().lancerMaxHealth
            << ", \"meridian_lancer_cooldown_ticks\": "
            << static_cast<std::uint64_t>(AuthoredRules().lancerCooldownTicks)
            << "},\n";
        out << "  \"map_grid_tiles\": 64,\n";
        out << "  \"map_layouts\": [";
        for (std::size_t index = 0; index < layouts.size(); ++index) {
            out << (index == 0 ? "" : ", ") << "\""
                << MapLayoutId(layouts[index]) << "\"";
        }
        out << "],\n";
        out << "  \"spec_bal_003_grid\": {\n";
        out << "    \"band\": [0.40, 0.60],\n";
        out << "    \"note\": \"One cell per non-mirror faction pairing per "
               "shipping map. Degenerate conditions (all matches finishing on "
               "the same tick) are excluded and counted separately; a cell with "
               "fewer than 30 decisive matches is reported as too few to judge "
               "rather than given a rate.\",\n";
        out << "    \"cells_in_band\": " << cellsInBand << ",\n";
        out << "    \"cells_out_of_band\": " << cellsOutOfBand << ",\n";
        out << "    \"cells_too_few_decisive\": " << cellsUnderpowered << ",\n";
        out << "    \"rows\": [\n";
        {
            std::size_t emittedCells = 0;
            for (const auto& [key, cell] : balanceCells) {
                const bool judged = !cell.mirror && cell.decisive >= 30;
                const double rate =
                    cell.decisive > 0
                        ? static_cast<double>(cell.faction0Wins) / cell.decisive
                        : 0.0;
                out << "      {\"map\": \"" << cell.map
                    << "\", \"pairing\": \"" << cell.pairing
                    << "\", \"mirror\": " << (cell.mirror ? "true" : "false")
                    << ", \"decisive\": " << cell.decisive
                    << ", \"faction_0_wins\": " << cell.faction0Wins
                    << ", \"draws\": " << cell.draws
                    << ", \"unresolved\": " << cell.unresolved
                    << ", \"excluded_degenerate\": " << cell.excludedDegenerate
                    << ", \"seat_0_wins\": " << cell.seat0Wins
                    << ", \"seat_1_wins\": " << cell.seat1Wins
                    << ", \"judged\": " << (judged ? "true" : "false");
                if (judged) {
                    out << ", \"faction_0_win_rate\": " << rate
                        << ", \"in_band\": "
                        << ((rate >= 0.40 && rate <= 0.60) ? "true" : "false");
                }
                out << "}"
                    << (++emittedCells == balanceCells.size() ? "\n" : ",\n");
            }
        }
        out << "    ]\n  },\n";
        out << "  \"total_matches\": " << totalMatches << ",\n";
        out << "  \"authoritative_terminal_matches\": "
            << (totalMatches - unresolved) << ",\n";
        out << "  \"actionable_stalls\": " << unresolved << ",\n";
        out << "  \"elapsed_seconds\": " << elapsedSec << ",\n";
        out << "  \"throughput_matches_per_sec\": " << matchesPerSec << ",\n";
        out << "  \"spawn_fairness\": {\n";
        out << "    \"slot_0_wins\": " << slot0Wins << ",\n";
        out << "    \"slot_1_wins\": " << slot1Wins << ",\n";
        out << "    \"draws\": " << draws << ",\n";
        out << "    \"rate\": " << spawnCI.rate << ",\n";
        out << "    \"margin_of_error\": " << spawnCI.marginOfError << ",\n";
        out << "    \"ci_lower\": " << spawnCI.lower << ",\n";
        out << "    \"ci_upper\": " << spawnCI.upper << ",\n";
        out << "    \"passed\": " << (spawnFairnessPassed ? "true" : "false") << "\n";
        out << "  },\n";
        out << "  \"asymmetry_balance\": {\n";
        out << "    \"meridian_vs_kharuun\": {\"rate\": " << mkCI.rate << ", \"ci_lower\": " << mkCI.lower << ", \"ci_upper\": " << mkCI.upper << ", \"margin\": " << mkCI.marginOfError << ", \"passed\": " << (mkPassed ? "true" : "false") << "},\n";
        out << "    \"meridian_vs_choir\": {\"rate\": " << mcCI.rate << ", \"ci_lower\": " << mcCI.lower << ", \"ci_upper\": " << mcCI.upper << ", \"margin\": " << mcCI.marginOfError << ", \"passed\": " << (mcPassed ? "true" : "false") << "},\n";
        out << "    \"kharuun_vs_choir\": {\"rate\": " << kcCI.rate << ", \"ci_lower\": " << kcCI.lower << ", \"ci_upper\": " << kcCI.upper << ", \"margin\": " << kcCI.marginOfError << ", \"passed\": " << (kcPassed ? "true" : "false") << "},\n";
        out << "    \"passed\": " << (balanceBandPassed ? "true" : "false") << "\n";
        out << "  },\n";
        out << "  \"strategy_primacy\": {\n";
        out << "    \"high_tier_wins\": " << primacyHighWins << ",\n";
        out << "    \"flawed_tier_wins\": " << primacyFlawedWins << ",\n";
        out << "    \"rate\": " << primacyCI.rate << ",\n";
        out << "    \"margin\": " << primacyCI.marginOfError << ",\n";
        out << "    \"passed\": " << (primacyPassed ? "true" : "false") << "\n";
        out << "  },\n";
        out << "  \"determinism\": {\"passed\": " << (determinismPassed ? "true" : "false") << "},\n";
        out << "  \"ai_competence_battery\": {\"implemented_checks\": "
            << implementedBatteryChecks << ", \"required_checks\": "
            << requiredBatteryChecks << ", \"implemented_checks_passed\": "
            << (batteryPassed ? "true" : "false")
            << ", \"qualified\": false},\n";
        out << "  \"condition_sampling\": {\n";
        out << "    \"conditions\": " << conditions.size() << ",\n";
        out << "    \"degenerate_conditions\": " << degenerateConditions << ",\n";
        out << "    \"degenerate_matches\": " << degenerateMatches << ",\n";
        out << "    \"effective_observations\": "
            << (static_cast<int>(conditions.size()) - degenerateConditions)
            << ",\n";
        out << "    \"note\": \"A condition whose matches all finish on the "
               "same tick is one match replayed; its rows are excluded from "
               "every rate and interval in this report.\",\n";
        out << "    \"rows\": [\n";
        std::size_t emitted = 0;
        for (const auto& [id, c] : conditions) {
            out << "      {\"condition\": \"" << c.id
                << "\", \"matches\": " << c.matches
                << ", \"distinct_finishing_ticks\": " << c.distinctTicks.size()
                << ", \"seat0_wins\": " << c.seat0Wins
                << ", \"seat1_wins\": " << c.seat1Wins
                << ", \"draws\": " << c.draws
                << ", \"unresolved\": " << c.unresolved
                << ", \"degenerate\": " << (c.Degenerate() ? "true" : "false")
                << "}" << (++emitted == conditions.size() ? "\n" : ",\n");
        }
        out << "    ]\n  },\n";
        // REL-AI-024. Reported as two counts, never as one rate: matches with
        // no opportunity say nothing about the play, and collapsing them into
        // a denominator would hide exactly the case the tournament map was.
        out << "  \"denial_play\": {\n";
        out << "    \"sample\": \"one observation per seat per simulated "
               "second\",\n";
        out << "    \"matches_with_opportunity\": " << denialOpportunityMatches
            << ",\n";
        out << "    \"matches_with_denial_achieved\": " << denialAchievedMatches
            << ",\n";
        out << "    \"opportunity_samples\": " << denialOpportunitySamples
            << ",\n";
        out << "    \"achieved_samples\": " << denialAchievedSamples << ",\n";
        out << "    \"note\": \"opportunity = the seat holds no Preserve Well "
               "while its opponent holds one that has begun paying; achieved = "
               "one of the seat's bodies also stands inside that Well's capture "
               "radius, which is what stops the income.\"\n";
        out << "  },\n";
        out << "  \"matches\": [\n";
        for (std::size_t index = 0; index < results.size(); ++index) {
            const MatchRecord& r = results[index];
            out << "    {\"seed\": " << r.seed
                << ", \"map_id\": \"" << r.mapId
                << "\", \"faction_0\": \"" << r.faction0
                << "\", \"faction_1\": \"" << r.faction1
                << "\", \"personality_0\": \"" << r.personality0
                << "\", \"personality_1\": \"" << r.personality1
                << "\", \"winner_player\": " << r.winnerPlayer
                << ", \"winner_faction\": \"" << r.winnerFaction
                << "\", \"outcome\": " << static_cast<int>(r.outcome)
                << ", \"terminal\": " << (r.terminal ? "true" : "false")
                << ", \"termination\": \"" << r.termination
                << "\", \"duration_ticks\": " << r.durationTicks
                << ", \"final_checksum\": " << r.finalChecksum
                << ", \"last_material_progress_tick\": "
                << r.lastMaterialProgressTick
                << ", \"core_hp\": [" << r.player0CoreHitPoints
                << ", " << r.player1CoreHitPoints << "]"
                << ", \"seat1_planned_first\": "
                << (r.seat1PlannedFirst ? "true" : "false")
                << ", \"condition\": \"" << r.conditionId
                << "\", \"denial_opportunity_samples\": "
                << r.denialOpportunityTicks
                << ", \"denial_achieved_samples\": " << r.denialAchievedTicks
                << ", \"stall_reason\": \"" << r.stallReason << "\"}"
                << (index + 1 == results.size() ? "\n" : ",\n");
        }
        out << "  ],\n";
        out << "  \"qualification_limitations\": ["
            << "\"synthetic single-map fixture is not the three shipping maps\", "
            << "\"one of four competence checks is implemented\"],\n";
        out << "  \"overall_passed\": false\n";
        out << "}\n";
        out.close();
        std::cout << "Report written to: " << outputPath << "\n";
    }

    // This legacy harness is diagnostic until it runs the shipping map set and
    // implements the complete four-part competence battery. Never publish a
    // synthetic or unresolved matrix as balance qualification.
    const bool overallSuccess = false;
    return overallSuccess ? 0 : 1;
}
