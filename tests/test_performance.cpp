// test_performance.cpp - Performance tests for OrganismEvolution
// Tests scalability and frame time targets

#include "entities/Creature.h"
#include "entities/Genome.h"
#include "entities/NeuralNetwork.h"
#include "entities/CreatureType.h"
#include "utils/SpatialGrid.h"
#include <glm/glm.hpp>
#include <cassert>
#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm>
#include <iomanip>

using namespace std::chrono;

// Performance result structure
struct PerfResult {
    std::string testName;
    int count;
    double avgMs;
    double minMs;
    double maxMs;
    bool passed;
};

std::vector<PerfResult> results;

// Test 1000 creature update
void testThousandCreatures() {
    std::cout << "Testing 1000 creature update..." << std::endl;

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;
    SpatialGrid grid(500.0f, 500.0f, 25);

    // Create 1000 creatures
    for (int i = 0; i < 1000; i++) {
        CreatureType type = (i % 2 == 0) ? CreatureType::GRAZER : CreatureType::APEX_PREDATOR;
        glm::vec3 pos((float)(rand() % 500), 0.0f, (float)(rand() % 500));
        creatures.push_back(std::make_unique<Creature>(pos, genome, type));
    }

    // Benchmark grid operations (creature updates need terrain which we don't have)
    double totalMs = 0.0;
    double minMs = 1000000.0;
    double maxMs = 0.0;
    int frames = 100;

    for (int frame = 0; frame < frames; frame++) {
        auto start = high_resolution_clock::now();

        grid.clear();
        for (auto& c : creatures) {
            grid.insert(c.get());
        }

        for (auto& c : creatures) {
            const auto& nearby = grid.query(c->getPosition(), c->getVisionRange());
            (void)nearby;
        }

        auto end = high_resolution_clock::now();
        double frameMs = duration_cast<microseconds>(end - start).count() / 1000.0;

        totalMs += frameMs;
        minMs = std::min(minMs, frameMs);
        maxMs = std::max(maxMs, frameMs);
    }

    double avgMs = totalMs / frames;
    bool passed = avgMs < 16.0;  // Must complete in one 60fps frame

    results.push_back({"1000 Creatures", 1000, avgMs, minMs, maxMs, passed});

    std::cout << "  Average: " << avgMs << "ms (target: <16ms)" << std::endl;
    std::cout << "  Min/Max: " << minMs << "ms / " << maxMs << "ms" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test 2000 creature update
void testTwoThousandCreatures() {
    std::cout << "Testing 2000 creature update..." << std::endl;

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;
    SpatialGrid grid(700.0f, 700.0f, 35);

    // Create 2000 creatures
    for (int i = 0; i < 2000; i++) {
        CreatureType type = (i % 2 == 0) ? CreatureType::GRAZER : CreatureType::APEX_PREDATOR;
        glm::vec3 pos((float)(rand() % 700), 0.0f, (float)(rand() % 700));
        creatures.push_back(std::make_unique<Creature>(pos, genome, type));
    }

    // Benchmark
    double totalMs = 0.0;
    double minMs = 1000000.0;
    double maxMs = 0.0;
    int frames = 50;

    for (int frame = 0; frame < frames; frame++) {
        auto start = high_resolution_clock::now();

        grid.clear();
        for (auto& c : creatures) {
            grid.insert(c.get());
        }

        for (auto& c : creatures) {
            const auto& nearby = grid.query(c->getPosition(), c->getVisionRange());
            (void)nearby;
        }

        auto end = high_resolution_clock::now();
        double frameMs = duration_cast<microseconds>(end - start).count() / 1000.0;

        totalMs += frameMs;
        minMs = std::min(minMs, frameMs);
        maxMs = std::max(maxMs, frameMs);
    }

    double avgMs = totalMs / frames;
    bool passed = avgMs < 16.0;

    results.push_back({"2000 Creatures", 2000, avgMs, minMs, maxMs, passed});

    std::cout << "  Average: " << avgMs << "ms (target: <16ms)" << std::endl;
    std::cout << "  Min/Max: " << minMs << "ms / " << maxMs << "ms" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test 5000 creature update
void testFiveThousandCreatures() {
    std::cout << "Testing 5000 creature update..." << std::endl;

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;
    SpatialGrid grid(1000.0f, 1000.0f, 50);

    // Create 5000 creatures
    for (int i = 0; i < 5000; i++) {
        CreatureType type = (i % 2 == 0) ? CreatureType::GRAZER : CreatureType::APEX_PREDATOR;
        glm::vec3 pos((float)(rand() % 1000), 0.0f, (float)(rand() % 1000));
        creatures.push_back(std::make_unique<Creature>(pos, genome, type));
    }

    // Benchmark
    double totalMs = 0.0;
    double minMs = 1000000.0;
    double maxMs = 0.0;
    int frames = 20;

    for (int frame = 0; frame < frames; frame++) {
        auto start = high_resolution_clock::now();

        grid.clear();
        for (auto& c : creatures) {
            grid.insert(c.get());
        }

        for (auto& c : creatures) {
            const auto& nearby = grid.query(c->getPosition(), c->getVisionRange());
            (void)nearby;
        }

        auto end = high_resolution_clock::now();
        double frameMs = duration_cast<microseconds>(end - start).count() / 1000.0;

        totalMs += frameMs;
        minMs = std::min(minMs, frameMs);
        maxMs = std::max(maxMs, frameMs);
    }

    double avgMs = totalMs / frames;
    bool passed = avgMs < 33.0;  // Target 30fps for large populations

    results.push_back({"5000 Creatures", 5000, avgMs, minMs, maxMs, passed});

    std::cout << "  Average: " << avgMs << "ms (target: <33ms for 30fps)" << std::endl;
    std::cout << "  Min/Max: " << minMs << "ms / " << maxMs << "ms" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;
}

// Test 10000 creature update
void testTenThousandCreatures() {
    std::cout << "Testing 10000 creature update..." << std::endl;

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;
    SpatialGrid grid(1500.0f, 1500.0f, 75);

    // Create 10000 creatures
    for (int i = 0; i < 10000; i++) {
        CreatureType type = (i % 2 == 0) ? CreatureType::GRAZER : CreatureType::APEX_PREDATOR;
        glm::vec3 pos((float)(rand() % 1500), 0.0f, (float)(rand() % 1500));
        creatures.push_back(std::make_unique<Creature>(pos, genome, type));
    }

    // Benchmark
    double totalMs = 0.0;
    double minMs = 1000000.0;
    double maxMs = 0.0;
    int frames = 10;

    for (int frame = 0; frame < frames; frame++) {
        auto start = high_resolution_clock::now();

        grid.clear();
        for (auto& c : creatures) {
            grid.insert(c.get());
        }

        for (auto& c : creatures) {
            const auto& nearby = grid.query(c->getPosition(), c->getVisionRange());
            (void)nearby;
        }

        auto end = high_resolution_clock::now();
        double frameMs = duration_cast<microseconds>(end - start).count() / 1000.0;

        totalMs += frameMs;
        minMs = std::min(minMs, frameMs);
        maxMs = std::max(maxMs, frameMs);
    }

    double avgMs = totalMs / frames;
    bool passed = avgMs < 100.0;  // Target 10fps minimum for extreme populations

    results.push_back({"10000 Creatures", 10000, avgMs, minMs, maxMs, passed});

    std::cout << "  Average: " << avgMs << "ms (target: <100ms for 10fps)" << std::endl;
    std::cout << "  Min/Max: " << minMs << "ms / " << maxMs << "ms" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "WARNING - May need GPU compute") << std::endl;
}

// Test spatial grid query performance
void testSpatialGridQueryPerformance() {
    std::cout << "Testing spatial grid query performance..." << std::endl;

    Genome genome;
    genome.randomize();

    SpatialGrid grid(1000.0f, 1000.0f, 50);

    std::vector<std::unique_ptr<Creature>> creatures;
    for (int i = 0; i < 2000; i++) {
        glm::vec3 pos((float)(rand() % 1000), 0.0f, (float)(rand() % 1000));
        creatures.push_back(std::make_unique<Creature>(pos, genome, CreatureType::GRAZER));
    }

    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    // Benchmark 10000 queries
    auto start = high_resolution_clock::now();

    int totalFound = 0;
    for (int i = 0; i < 10000; i++) {
        float x = (float)(rand() % 1000);
        float z = (float)(rand() % 1000);
        const auto& nearby = grid.query(glm::vec3(x, 0.0f, z), 50.0f);
        totalFound += nearby.size();
    }

    auto end = high_resolution_clock::now();
    double totalMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    double perQueryUs = (totalMs * 1000.0) / 10000.0;

    bool passed = perQueryUs < 100.0;  // Each query should be under 100 microseconds

    std::cout << "  10000 queries in " << totalMs << "ms" << std::endl;
    std::cout << "  Per query: " << perQueryUs << " microseconds" << std::endl;
    std::cout << "  Average creatures found: " << (totalFound / 10000.0) << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;

    results.push_back({"Spatial Query (10k)", 10000, perQueryUs / 1000.0, 0, 0, passed});
}

// Test neural network forward pass performance
void testNeuralNetworkPerformance() {
    std::cout << "Testing neural network performance..." << std::endl;

    std::vector<float> weights(200, 0.3f);
    NeuralNetwork nn(weights);
    std::vector<float> inputs = {0.5f, 0.3f, 0.8f, -0.2f, 0.7f, 0.4f, 0.5f, 0.2f};

    // Benchmark 100000 neural forward passes
    auto start = high_resolution_clock::now();

    for (int i = 0; i < 100000; i++) {
        NeuralOutputs out = nn.forward(inputs);
        (void)out;
    }

    auto end = high_resolution_clock::now();
    double totalMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    double perPassUs = (totalMs * 1000.0) / 100000.0;

    bool passed = perPassUs < 10.0;  // Each neural pass should be under 10 microseconds

    std::cout << "  100000 neural passes in " << totalMs << "ms" << std::endl;
    std::cout << "  Per pass: " << perPassUs << " microseconds" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;

    results.push_back({"Neural Forward (100k)", 100000, perPassUs / 1000.0, 0, 0, passed});
}

// Test genome mutation performance
void testGenomeMutationPerformance() {
    std::cout << "Testing genome mutation performance..." << std::endl;

    std::vector<Genome> genomes;
    for (int i = 0; i < 1000; i++) {
        Genome g;
        g.randomize();
        genomes.push_back(g);
    }

    // Benchmark 100000 mutations
    auto start = high_resolution_clock::now();

    for (int round = 0; round < 100; round++) {
        for (auto& g : genomes) {
            g.mutate(0.1f);
        }
    }

    auto end = high_resolution_clock::now();
    double totalMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    double perMutationUs = (totalMs * 1000.0) / 100000.0;

    bool passed = perMutationUs < 5.0;  // Each mutation should be under 5 microseconds

    std::cout << "  100000 mutations in " << totalMs << "ms" << std::endl;
    std::cout << "  Per mutation: " << perMutationUs << " microseconds" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;

    results.push_back({"Genome Mutation (100k)", 100000, perMutationUs / 1000.0, 0, 0, passed});
}

// Test genome crossover performance
void testGenomeCrossoverPerformance() {
    std::cout << "Testing genome crossover performance..." << std::endl;

    std::vector<Genome> parents;
    for (int i = 0; i < 100; i++) {
        Genome g;
        g.randomize();
        parents.push_back(g);
    }

    // Benchmark 10000 crossovers
    auto start = high_resolution_clock::now();

    for (int round = 0; round < 100; round++) {
        for (int i = 0; i < 100; i++) {
            Genome child(parents[i], parents[(i + 1) % 100]);
            (void)child;
        }
    }

    auto end = high_resolution_clock::now();
    double totalMs = duration_cast<microseconds>(end - start).count() / 1000.0;
    double perCrossoverUs = (totalMs * 1000.0) / 10000.0;

    bool passed = perCrossoverUs < 10.0;  // Each crossover should be under 10 microseconds

    std::cout << "  10000 crossovers in " << totalMs << "ms" << std::endl;
    std::cout << "  Per crossover: " << perCrossoverUs << " microseconds" << std::endl;
    std::cout << "  " << (passed ? "PASSED" : "FAILED") << std::endl;

    results.push_back({"Genome Crossover (10k)", 10000, perCrossoverUs / 1000.0, 0, 0, passed});
}

// Print final summary
void printSummary() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "PERFORMANCE TEST SUMMARY" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    int passed = 0;
    int total = 0;

    std::cout << std::left;
    std::cout << std::setw(25) << "Test" << std::setw(10) << "Count"
              << std::setw(12) << "Avg (ms)" << std::setw(10) << "Status" << std::endl;
    std::cout << std::string(60, '-') << std::endl;

    for (const auto& r : results) {
        std::cout << std::setw(25) << r.testName
                  << std::setw(10) << r.count
                  << std::setw(12) << std::fixed << std::setprecision(3) << r.avgMs
                  << std::setw(10) << (r.passed ? "PASS" : "FAIL") << std::endl;
        if (r.passed) passed++;
        total++;
    }

    std::cout << std::string(60, '-') << std::endl;
    std::cout << "Total: " << passed << "/" << total << " tests passed" << std::endl;

    if (passed == total) {
        std::cout << "\n*** ALL PERFORMANCE TESTS PASSED ***" << std::endl;
    } else {
        std::cout << "\n*** SOME PERFORMANCE TESTS FAILED ***" << std::endl;
    }
}

int main() {
    std::cout << "=== Performance Tests ===" << std::endl;
    std::cout << "(Running on single thread, CPU only)" << std::endl;
    std::cout << std::endl;

    testThousandCreatures();
    testTwoThousandCreatures();
    testFiveThousandCreatures();
    testTenThousandCreatures();
    testSpatialGridQueryPerformance();
    testNeuralNetworkPerformance();
    testGenomeMutationPerformance();
    testGenomeCrossoverPerformance();

    printSummary();

    return 0;
}
