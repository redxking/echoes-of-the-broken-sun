#include "EchoesSimCore/Simulation.h"

#include <algorithm>
#include <array>
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
#include <thread>
#include <vector>

namespace echoes::balance {

using namespace echoes::sim;

struct MatchRecord {
    std::uint64_t seed = 0;
    std::string mapId = "TournamentSymmetric48";
    std::string faction0;
    std::string faction1;
    std::int32_t slot0 = 0;
    std::int32_t slot1 = 1;
    std::string personality0;
    std::string personality1;
    std::string winnerFaction;
    std::int32_t winnerPlayer = -1;  // 0, 1, or -1 (Draw)
    Tick durationTicks = 0;
    std::uint64_t finalChecksum = 0;
};

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

    // Player 0 base at (10, 10)
    sim.SpawnEntity(0, f0, EntityType::CommandCore, Vec2::FromTiles(10, 10));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(12, 10));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(10, 12));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(11, 9));
    sim.SpawnEntity(0, f0, EntityType::Worker, Vec2::FromTiles(9, 11));
    sim.SpawnResourceNode(Vec2::FromTiles(6, 10), 10000);
    sim.SpawnFutureWell(Vec2::FromTiles(10, 6));

    // Player 1 base at (38, 38)
    sim.SpawnEntity(1, f1, EntityType::CommandCore, Vec2::FromTiles(38, 38));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(36, 38));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(38, 36));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(37, 39));
    sim.SpawnEntity(1, f1, EntityType::Worker, Vec2::FromTiles(39, 37));
    sim.SpawnResourceNode(Vec2::FromTiles(42, 38), 10000);
    sim.SpawnFutureWell(Vec2::FromTiles(38, 42));

    // Contested neutral center — resources only, no well (wells are per-base)
    sim.SpawnResourceNode(Vec2::FromTiles(20, 24), 8000);
    sim.SpawnResourceNode(Vec2::FromTiles(28, 24), 8000);
    sim.SpawnResourceNode(Vec2::FromTiles(24, 20), 6000);
    sim.SpawnResourceNode(Vec2::FromTiles(24, 28), 6000);
}

MatchRecord RunMatch(std::uint64_t seed,
                     Faction f0,
                     Faction f1,
                     AiPersonality p0,
                     AiPersonality p1,
                     Tick maxTicks = 12000) {
    Simulation sim(SimulationConfig{48, 48, 20, seed});
    SetupTournamentMap(sim, f0, f1);

    MatchRecord record{};
    record.seed = seed;
    record.faction0 = FactionToString(f0);
    record.faction1 = FactionToString(f1);
    record.personality0 = PersonalityToString(p0);
    record.personality1 = PersonalityToString(p1);

    Tick ticks = 0;
    while (sim.Outcome() == MatchOutcome::Ongoing && ticks < maxTicks) {
        if (ticks % 4 == 0) {
            const auto cmds0 = sim.GenerateAiCommands(0, p0);
            for (const auto& c : cmds0) {
                sim.QueueCommand(c);
            }
            const auto cmds1 = sim.GenerateAiCommands(1, p1);
            for (const auto& c : cmds1) {
                sim.QueueCommand(c);
            }
        }
        sim.Step();
        ++ticks;
    }

    record.durationTicks = ticks;
    record.finalChecksum = sim.StateChecksum();

    const MatchOutcome outcome = sim.Outcome();
    if (outcome == MatchOutcome::Player0Victory) {
        record.winnerPlayer = 0;
        record.winnerFaction = record.faction0;
    } else if (outcome == MatchOutcome::Player1Victory) {
        record.winnerPlayer = 1;
        record.winnerFaction = record.faction1;
    } else {
        // Evaluate by surviving Command Core hit points, then surviving entity HP
        std::int32_t p0CoreHp = 0;
        std::int32_t p1CoreHp = 0;
        std::int32_t p0TotalHp = 0;
        std::int32_t p1TotalHp = 0;
        for (const auto& e : sim.Entities()) {
            if (e.owner == 0) {
                p0TotalHp += e.hitPoints;
                if (e.type == EntityType::CommandCore) {
                    p0CoreHp += e.hitPoints;
                }
            } else if (e.owner == 1) {
                p1TotalHp += e.hitPoints;
                if (e.type == EntityType::CommandCore) {
                    p1CoreHp += e.hitPoints;
                }
            }
        }
        if (p0CoreHp > p1CoreHp || (p0CoreHp == p1CoreHp && p0TotalHp > p1TotalHp)) {
            record.winnerPlayer = 0;
            record.winnerFaction = record.faction0;
        } else if (p1CoreHp > p0CoreHp || (p0CoreHp == p1CoreHp && p1TotalHp > p0TotalHp)) {
            record.winnerPlayer = 1;
            record.winnerFaction = record.faction1;
        } else {
            const auto* pl0 = sim.FindPlayer(0);
            const auto* pl1 = sim.FindPlayer(1);
            const int res0 = pl0 ? (pl0->resources.material + pl0->resources.dawnshards * 2) : 0;
            const int res1 = pl1 ? (pl1->resources.material + pl1->resources.dawnshards * 2) : 0;
            if (res0 > res1) {
                record.winnerPlayer = 0;
                record.winnerFaction = record.faction0;
            } else if (res1 > res0) {
                record.winnerPlayer = 1;
                record.winnerFaction = record.faction1;
            } else {
                // True tie — use seed parity as a fair coin flip.
                const int coin = static_cast<int>(seed & 1);
                record.winnerPlayer = coin;
                record.winnerFaction = coin == 0 ? record.faction0 : record.faction1;
            }
        }
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
        }
    }

    std::cout << "========================================================\n";
    std::cout << "Echoes of the Broken Sun — Headless AI Balance Harness\n";
    std::cout << "SPEC-BAL-001..008 Automated 1,000-Match Validation Matrix\n";
    std::cout << "========================================================\n";
    std::cout << "Target Matches: " << totalMatches << " | Threads: " << requestedThreads << "\n";

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
    };

    std::vector<MatchTask> tasks;
    tasks.reserve(totalMatches);

    for (int i = 0; i < totalMatches; ++i) {
        const int matchupIndex = i % 9;
        const Faction f0 = kFactions[matchupIndex / 3];
        const Faction f1 = kFactions[matchupIndex % 3];
        tasks.push_back({
            baseSeed + static_cast<std::uint64_t>(i) * 10007ULL,
            f0,
            f1,
            AiPersonality::Adaptive,
            AiPersonality::Adaptive,
        });
    }

    std::vector<MatchRecord> results(totalMatches);
    std::mutex progressMutex;
    std::atomic<int> completedTasks{0};

    auto worker = [&](int threadId) {
        for (int idx = threadId; idx < totalMatches; idx += requestedThreads) {
            const auto& task = tasks[idx];
            MatchRecord rec = RunMatch(task.seed, task.f0, task.f1, task.p0, task.p1);
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

    // 1. Evaluate Spawn Slot Fairness (SPEC-BAL-004)
    int slot0Wins = 0;
    int slot1Wins = 0;
    int draws = 0;
    for (const auto& r : results) {
        if (r.winnerPlayer == 0) slot0Wins++;
        else if (r.winnerPlayer == 1) slot1Wins++;
        else draws++;
    }
    const int decisiveMatches = slot0Wins + slot1Wins;
    const auto spawnCI = ComputeConfidenceInterval(slot0Wins, decisiveMatches);
    const bool spawnFairnessPassed = (spawnCI.rate >= 0.45 && spawnCI.rate <= 0.55);

    // 2. Evaluate Non-Mirror Pairings Balance Band (SPEC-BAL-003)
    struct PairStats {
        int winsA = 0;
        int winsB = 0;
        int pairDraws = 0;
    };
    std::array<PairStats, 3> nonMirrorPairs; // 0: M vs K, 1: M vs C, 2: K vs C

    for (const auto& r : results) {
        if (r.faction0 == "MeridianCompact" && r.faction1 == "KharuunAssemblies") {
            if (r.winnerPlayer == 0) nonMirrorPairs[0].winsA++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[0].winsB++;
            else nonMirrorPairs[0].pairDraws++;
        } else if (r.faction0 == "KharuunAssemblies" && r.faction1 == "MeridianCompact") {
            if (r.winnerPlayer == 0) nonMirrorPairs[0].winsB++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[0].winsA++;
            else nonMirrorPairs[0].pairDraws++;
        } else if (r.faction0 == "MeridianCompact" && r.faction1 == "HollowChoir") {
            if (r.winnerPlayer == 0) nonMirrorPairs[1].winsA++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[1].winsB++;
            else nonMirrorPairs[1].pairDraws++;
        } else if (r.faction0 == "HollowChoir" && r.faction1 == "MeridianCompact") {
            if (r.winnerPlayer == 0) nonMirrorPairs[1].winsB++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[1].winsA++;
            else nonMirrorPairs[1].pairDraws++;
        } else if (r.faction0 == "KharuunAssemblies" && r.faction1 == "HollowChoir") {
            if (r.winnerPlayer == 0) nonMirrorPairs[2].winsA++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[2].winsB++;
            else nonMirrorPairs[2].pairDraws++;
        } else if (r.faction0 == "HollowChoir" && r.faction1 == "KharuunAssemblies") {
            if (r.winnerPlayer == 0) nonMirrorPairs[2].winsB++;
            else if (r.winnerPlayer == 1) nonMirrorPairs[2].winsA++;
            else nonMirrorPairs[2].pairDraws++;
        }
    }

    const auto mkCI = ComputeConfidenceInterval(nonMirrorPairs[0].winsA, nonMirrorPairs[0].winsA + nonMirrorPairs[0].winsB);
    const auto mcCI = ComputeConfidenceInterval(nonMirrorPairs[1].winsA, nonMirrorPairs[1].winsA + nonMirrorPairs[1].winsB);
    const auto kcCI = ComputeConfidenceInterval(nonMirrorPairs[2].winsA, nonMirrorPairs[2].winsA + nonMirrorPairs[2].winsB);

    const bool mkPassed = (mkCI.rate >= 0.40 && mkCI.rate <= 0.60);
    const bool mcPassed = (mcCI.rate >= 0.40 && mcCI.rate <= 0.60);
    const bool kcPassed = (kcCI.rate >= 0.40 && kcCI.rate <= 0.60);
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
            const MatchRecord replay = RunMatch(sample.seed, f0, f1, AiPersonality::Adaptive, AiPersonality::Adaptive);
            if (replay.durationTicks != sample.durationTicks ||
                replay.finalChecksum != sample.finalChecksum ||
                replay.winnerPlayer != sample.winnerPlayer) {
                determinismPassed = false;
                std::cerr << "DETERMINISM VIOLATION on seed " << sample.seed << "\n";
            }
        }
    }

    // 5. AI Competence Battery (SPEC-BAL-008)
    bool batteryPassed = true;
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
              << std::fixed << std::setprecision(1) << (spawnCI.rate * 100.0) << "% ± "
              << (spawnCI.marginOfError * 100.0) << "% (N=" << decisiveMatches << ") "
              << (spawnFairnessPassed ? "[PASS]" : "[FAIL]") << "\n";
    std::cout << "Meridian vs Kharuun:              "
              << (mkCI.rate * 100.0) << "% ± " << (mkCI.marginOfError * 100.0)
              << "% (N=" << (nonMirrorPairs[0].winsA + nonMirrorPairs[0].winsB) << ") "
              << (mkPassed ? "[PASS]" : "[FAIL]") << "\n";
    std::cout << "Meridian vs Hollow Choir:         "
              << (mcCI.rate * 100.0) << "% ± " << (mcCI.marginOfError * 100.0)
              << "% (N=" << (nonMirrorPairs[1].winsA + nonMirrorPairs[1].winsB) << ") "
              << (mcPassed ? "[PASS]" : "[FAIL]") << "\n";
    std::cout << "Kharuun vs Hollow Choir:          "
              << (kcCI.rate * 100.0) << "% ± " << (kcCI.marginOfError * 100.0)
              << "% (N=" << (nonMirrorPairs[2].winsA + nonMirrorPairs[2].winsB) << ") "
              << (kcPassed ? "[PASS]" : "[FAIL]") << "\n";
    std::cout << "Strategy Primacy (Adaptive vs Econ): "
              << (primacyCI.rate * 100.0) << "% ± " << (primacyCI.marginOfError * 100.0)
              << "% (N=" << (primacyHighWins + primacyFlawedWins) << ") "
              << (primacyPassed ? "[PASS]" : "[FAIL]") << "\n";
    std::cout << "Replay Determinism:               "
              << (determinismPassed ? "10/10 Bit-Exact [PASS]" : "Divergence Detected [FAIL]") << "\n";
    std::cout << "AI Competence Battery:            "
              << (batteryPassed ? "Passed [PASS]" : "Failed [FAIL]") << "\n";
    std::cout << "=================================================\n";

    // Write structured JSON
    std::ofstream out(outputPath);
    if (out.is_open()) {
        out << "{\n";
        out << "  \"total_matches\": " << totalMatches << ",\n";
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
        out << "  \"ai_competence_battery\": {\"passed\": " << (batteryPassed ? "true" : "false") << "},\n";
        out << "  \"overall_passed\": " << (spawnFairnessPassed && balanceBandPassed && primacyPassed && determinismPassed && batteryPassed ? "true" : "false") << "\n";
        out << "}\n";
        out.close();
        std::cout << "Report written to: " << outputPath << "\n";
    }

    const bool overallSuccess = spawnFairnessPassed && balanceBandPassed && primacyPassed && determinismPassed && batteryPassed;
    return overallSuccess ? 0 : 1;
}
