#include "EchoesSimCore/Simulation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {

using namespace echoes::sim;
using Clock = std::chrono::steady_clock;

struct Measurement final {
    double averageMs = 0.0;
    double p95Ms = 0.0;
    double maximumMs = 0.0;
};

[[nodiscard]] bool IsGlassScarCrossing(std::int32_t tileX) {
    return (tileX >= 12 && tileX <= 15) ||
           (tileX >= 29 && tileX <= 35) ||
           (tileX >= 48 && tileX <= 51);
}

void ConfigureGlassScar(Simulation& simulation) {
    std::int32_t blocked = 0;
    for (std::int32_t tileY = 30; tileY <= 34; ++tileY) {
        for (std::int32_t tileX = 8; tileX <= 55; ++tileX) {
            if (!IsGlassScarCrossing(tileX) &&
                simulation.SetTerrainTile(tileX, tileY, Terrain::Blocked)) {
                ++blocked;
            }
        }
    }
    if (blocked != 165) {
        throw std::runtime_error("Glass Scar profile topology is invalid");
    }
}

struct ScaleFixture final {
    Simulation simulation{{64, 64, 20, 0x50524f46494c4531ULL}};
    std::vector<EntityId> southernMeridian{};
};

[[nodiscard]] ScaleFixture MakeScaleFixture() {
    ScaleFixture fixture{};
    ConfigureGlassScar(fixture.simulation);
    if (!fixture.simulation.AddPlayer(
            0, Faction::MeridianCompact, {0, 0}) ||
        !fixture.simulation.AddPlayer(
            1, Faction::KharuunAssemblies, {0, 0}) ||
        !fixture.simulation.AddPlayer(
            2, Faction::KharuunAssemblies, {0, 0}) ||
        !fixture.simulation.AddPlayer(
            3, Faction::MeridianCompact, {0, 0})) {
        throw std::runtime_error("profile players could not be created");
    }

    std::size_t spawned = 0;
    const auto spawnTeam = [&](PlayerId player,
                               Faction faction,
                               std::int32_t minimumX,
                               std::int32_t maximumX,
                               std::int32_t minimumY,
                               std::int32_t maximumY,
                               bool collectForMovement) {
        std::size_t teamSpawned = 0;
        for (std::int32_t tileY = minimumY;
             tileY <= maximumY && teamSpawned < 100; tileY += 2) {
            for (std::int32_t tileX = minimumX;
                 tileX <= maximumX && teamSpawned < 100; tileX += 2) {
                const EntityId id = fixture.simulation.SpawnEntity(
                    player, faction, EntityType::Soldier,
                    Vec2::FromTiles(tileX, tileY));
                if (id == 0) {
                    throw std::runtime_error("four-team profile unit spawn failed");
                }
                if (collectForMovement) {
                    fixture.southernMeridian.push_back(id);
                }
                ++teamSpawned;
                ++spawned;
            }
        }
        if (teamSpawned != 100) {
            throw std::runtime_error("four-team profile quadrant is undersized");
        }
    };
    spawnTeam(0, Faction::MeridianCompact, 2, 30, 2, 28, true);
    spawnTeam(2, Faction::KharuunAssemblies, 34, 62, 2, 28, false);
    spawnTeam(3, Faction::MeridianCompact, 2, 30, 36, 62, false);
    spawnTeam(1, Faction::KharuunAssemblies, 34, 62, 36, 62, false);
    if (spawned != 400 || fixture.southernMeridian.size() != 100) {
        throw std::runtime_error("profile fixture did not reach 400 units");
    }
    return fixture;
}

template <typename Operation>
[[nodiscard]] Measurement Measure(std::size_t warmupCount,
                                  std::size_t sampleCount,
                                  Operation operation,
                                  double operationsPerSample = 1.0) {
    for (std::size_t index = 0; index < warmupCount; ++index) {
        operation();
    }
    std::vector<double> samples{};
    samples.reserve(sampleCount);
    for (std::size_t index = 0; index < sampleCount; ++index) {
        const Clock::time_point start = Clock::now();
        operation();
        const Clock::time_point end = Clock::now();
        samples.push_back(
            std::chrono::duration<double, std::milli>(end - start).count() /
            operationsPerSample);
    }
    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t p95Index = static_cast<std::size_t>(
        std::ceil(static_cast<double>(sorted.size()) * 0.95)) - 1;
    return {
        std::accumulate(samples.begin(), samples.end(), 0.0) /
            static_cast<double>(samples.size()),
        sorted[p95Index],
        sorted.back()};
}

void WriteMeasurement(const char* name,
                      const Measurement& measurement,
                      bool trailingComma) {
    std::cout << "    \"" << name << "\": {\"average_ms\": "
              << measurement.averageMs << ", \"p95_ms\": "
              << measurement.p95Ms << ", \"maximum_ms\": "
              << measurement.maximumMs << "}"
              << (trailingComma ? "," : "") << "\n";
}

}  // namespace

int main() {
    try {
        ScaleFixture visibilityFixture = MakeScaleFixture();
        const Measurement visibility = Measure(50, 1000, [&]() {
            visibilityFixture.simulation.ProfileRefreshVisibility();
        });

        std::vector<Vec2> pathStarts{};
        std::vector<Vec2> pathDestinations{};
        pathStarts.reserve(100);
        pathDestinations.reserve(100);
        constexpr std::int32_t crossingCenters[] = {13, 32, 49};
        for (std::int32_t index = 0; index < 100; ++index) {
            const std::int32_t startX = 2 + (index * 7) % 60;
            const std::int32_t destinationX = crossingCenters[index % 3];
            pathStarts.push_back(Vec2::FromTiles(startX, 6 + index % 20));
            pathDestinations.push_back(
                Vec2::FromTiles(destinationX, 56 - index % 18));
        }
        constexpr std::size_t coldPathSampleCount = 30;
        std::vector<Simulation> coldPathFixtures{};
        coldPathFixtures.reserve(coldPathSampleCount);
        for (std::size_t index = 0; index < coldPathSampleCount; ++index) {
            coldPathFixtures.emplace_back(SimulationConfig{
                64, 64, 20, 0x50524f46494c4532ULL + index});
            ConfigureGlassScar(coldPathFixtures.back());
        }
        std::uint64_t coldCompletedPathRequests = 0;
        std::size_t coldPathSample = 0;
        const Measurement coldPathBurst = Measure(0, coldPathSampleCount, [&]() {
            for (std::size_t index = 0; index < pathStarts.size(); ++index) {
                if (coldPathFixtures[coldPathSample].ProfilePathRequest(
                        pathStarts[index], pathDestinations[index])) {
                    ++coldCompletedPathRequests;
                }
            }
            ++coldPathSample;
        });
        if (coldCompletedPathRequests !=
            coldPathSampleCount * pathStarts.size()) {
            throw std::runtime_error("one or more cold profile path requests failed");
        }

        std::uint64_t completedPathRequests = 0;
        const Measurement cachedPathBurst = Measure(10, 200, [&]() {
            for (std::size_t index = 0; index < pathStarts.size(); ++index) {
                if (visibilityFixture.simulation.ProfilePathRequest(
                        pathStarts[index], pathDestinations[index])) {
                    ++completedPathRequests;
                }
            }
        });
        if (completedPathRequests != 21000) {
            throw std::runtime_error("one or more profile path requests failed");
        }

        ScaleFixture tickFixture = MakeScaleFixture();
        for (std::size_t index = 0; index < 100; ++index) {
            Command move{};
            move.executeTick = 0;
            move.player = 0;
            move.sequence = index + 1;
            move.type = CommandType::Move;
            move.actor = tickFixture.southernMeridian[index];
            move.position = Vec2::FromTiles(
                crossingCenters[index % 3], 56 - static_cast<std::int32_t>(index % 18));
            if (!tickFixture.simulation.QueueCommand(move)) {
                throw std::runtime_error("profile movement command was rejected");
            }
        }
        const Measurement firstCommandTick = Measure(0, 1, [&]() {
            tickFixture.simulation.Step();
        });
        const Measurement simulationTick = Measure(5, 300, [&]() {
            tickFixture.simulation.Step();
        });

        std::uint64_t checksumSink = 0;
        constexpr std::size_t checksumBatchSize = 31;
        const Measurement checksum = Measure(10, 200, [&]() {
            for (std::size_t index = 0; index < checksumBatchSize; ++index) {
                checksumSink += tickFixture.simulation.StateChecksum();
            }
        }, static_cast<double>(checksumBatchSize));

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "{\n";
        std::cout << "  \"profile\": \"native-glass-scar-scale-v2\",\n";
        std::cout << "  \"boundary\": {\"map_tiles\": \"64x64\", "
                     "\"units\": 400, \"teams\": 4, "
                     "\"sim_hz\": 20, \"path_requests\": 100, "
                     "\"checksum_batch_size\": 31},\n";
        std::cout << "  \"measurements\": {\n";
        WriteMeasurement("visibility_refresh", visibility, true);
        WriteMeasurement("path_burst_100_cold", coldPathBurst, true);
        WriteMeasurement("path_burst_100_cached", cachedPathBurst, true);
        WriteMeasurement("first_tick_applying_100_move_commands", firstCommandTick, true);
        WriteMeasurement("simulation_tick_400_units_100_moving", simulationTick, true);
        WriteMeasurement("state_checksum_400_units", checksum, false);
        std::cout << "  },\n";
        std::cout << "  \"budgets_ms_p95\": {\"visibility_refresh\": 1.5, "
                     "\"path_burst_100_cold\": 6.0, "
                     "\"simulation_tick\": 4.0, "
                     "\"state_checksum\": 0.25},\n";
        std::cout << "  \"budget_results\": {\n";
        std::cout << "    \"visibility_four_team_pass\": "
                  << (visibility.p95Ms <= 1.5 ? "true" : "false") << ",\n";
        std::cout << "    \"path_burst_cold_pass\": "
                  << (coldPathBurst.p95Ms <= 6.0 ? "true" : "false") << ",\n";
        std::cout << "    \"simulation_tick_pass\": "
                  << (simulationTick.p95Ms <= 4.0 ? "true" : "false") << ",\n";
        std::cout << "    \"state_checksum_pass\": "
                  << (checksum.p95Ms <= 0.25 ? "true" : "false") << "\n";
        std::cout << "  },\n";
        std::cout << "  \"qualification\": "
                     "\"local optimized native measurement; not Unreal frame, "
                     "GPU, rendered-unit, networked-multiplayer, or release qualification\",\n";
        std::cout << "  \"checksum_sink\": " << checksumSink << "\n";
        std::cout << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "profile failed: " << error.what() << "\n";
        return 1;
    }
}
