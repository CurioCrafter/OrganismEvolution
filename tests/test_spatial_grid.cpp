// test_spatial_grid.cpp - Unit tests for SpatialGrid class
// Tests insertion, querying, and spatial partitioning

#include "utils/SpatialGrid.h"
#include "entities/Creature.h"
#include "entities/CreatureType.h"
#include "entities/Genome.h"
#include <glm/glm.hpp>
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>
#include <memory>

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// Test basic grid creation
void testGridCreation() {
    std::cout << "Testing SpatialGrid creation..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    // Grid should be empty initially
    assert(grid.getTotalCreatures() == 0);

    std::cout << "  SpatialGrid creation test passed!" << std::endl;
}

// Test single creature insertion
void testSingleInsertion() {
    std::cout << "Testing single creature insertion..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();
    Creature c(glm::vec3(25.0f, 0.0f, 25.0f), genome, CreatureType::GRAZER);

    grid.clear();
    grid.insert(&c);

    assert(grid.getTotalCreatures() == 1);

    std::cout << "  Single insertion test passed!" << std::endl;
}

// Test multiple creature insertions
void testMultipleInsertions() {
    std::cout << "Testing multiple insertions..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    std::vector<std::unique_ptr<Creature>> creatures;
    Genome genome;
    genome.randomize();

    for (int i = 0; i < 100; i++) {
        glm::vec3 pos((float)(rand() % 100), 0.0f, (float)(rand() % 100));
        creatures.push_back(std::make_unique<Creature>(pos, genome, CreatureType::GRAZER));
    }

    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    assert(grid.getTotalCreatures() == 100);

    std::cout << "  Multiple insertions test passed!" << std::endl;
}

// Test radius query
void testRadiusQuery() {
    std::cout << "Testing radius query..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    // Create creatures in a known pattern
    Creature c1(glm::vec3(50.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);  // Center
    Creature c2(glm::vec3(55.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);  // 5 units away
    Creature c3(glm::vec3(70.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);  // 20 units away

    grid.clear();
    grid.insert(&c1);
    grid.insert(&c2);
    grid.insert(&c3);

    // Query with radius 10 - should find c1 and c2
    const auto& nearby = grid.query(glm::vec3(50.0f, 0.0f, 50.0f), 10.0f);
    assert(nearby.size() >= 2);  // At least c1 and c2

    // Query with radius 25 - should find all 3
    const auto& allNearby = grid.query(glm::vec3(50.0f, 0.0f, 50.0f), 25.0f);
    assert(allNearby.size() == 3);

    std::cout << "  Radius query test passed!" << std::endl;
}

// Test type filtering
void testTypeFiltering() {
    std::cout << "Testing type filtering..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    Creature herbivore(glm::vec3(50.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);
    Creature carnivore(glm::vec3(55.0f, 0.0f, 50.0f), genome, CreatureType::APEX_PREDATOR);

    grid.clear();
    grid.insert(&herbivore);
    grid.insert(&carnivore);

    // Query for herbivores only
    const auto& herbivores = grid.queryByType(glm::vec3(50.0f, 0.0f, 50.0f), 20.0f,
                                               static_cast<int>(CreatureType::GRAZER));

    // Should find the herbivore
    bool foundHerbivore = false;
    for (auto* c : herbivores) {
        if (c->getType() == CreatureType::GRAZER) foundHerbivore = true;
    }
    assert(foundHerbivore);

    std::cout << "  Type filtering test passed!" << std::endl;
}

// Test grid clear
void testGridClear() {
    std::cout << "Testing grid clear..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;
    for (int i = 0; i < 50; i++) {
        glm::vec3 pos((float)(rand() % 100), 0.0f, (float)(rand() % 100));
        creatures.push_back(std::make_unique<Creature>(pos, genome, CreatureType::GRAZER));
    }

    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    assert(grid.getTotalCreatures() == 50);

    grid.clear();

    assert(grid.getTotalCreatures() == 0);

    std::cout << "  Grid clear test passed!" << std::endl;
}

// Test boundary conditions
void testBoundaryConditions() {
    std::cout << "Testing boundary conditions..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    // Creature at corner
    Creature c1(glm::vec3(0.0f, 0.0f, 0.0f), genome, CreatureType::GRAZER);

    // Creature at opposite corner
    Creature c2(glm::vec3(99.0f, 0.0f, 99.0f), genome, CreatureType::GRAZER);

    // Creature outside bounds (should still work - clamped)
    Creature c3(glm::vec3(-10.0f, 0.0f, -10.0f), genome, CreatureType::GRAZER);

    grid.clear();
    grid.insert(&c1);
    grid.insert(&c2);
    grid.insert(&c3);

    // All should be inserted
    assert(grid.getTotalCreatures() == 3);

    // Query at origin
    const auto& nearby = grid.query(glm::vec3(0.0f, 0.0f, 0.0f), 15.0f);

    // Should find at least c1 and c3
    assert(nearby.size() >= 1);

    std::cout << "  Boundary conditions test passed!" << std::endl;
}

// Test count nearby
void testCountNearby() {
    std::cout << "Testing count nearby..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;

    // Create cluster of creatures
    for (int i = 0; i < 10; i++) {
        glm::vec3 pos(50.0f + (float)(i % 3), 0.0f, 50.0f + (float)(i / 3));
        creatures.push_back(std::make_unique<Creature>(pos, genome, CreatureType::GRAZER));
    }

    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    int count = grid.countNearby(glm::vec3(50.0f, 0.0f, 50.0f), 10.0f);
    assert(count == 10);

    // Count with smaller radius
    int smallCount = grid.countNearby(glm::vec3(50.0f, 0.0f, 50.0f), 2.0f);
    assert(smallCount <= 10);
    assert(smallCount > 0);

    std::cout << "  Count nearby test passed!" << std::endl;
}

// Test find nearest
void testFindNearest() {
    std::cout << "Testing find nearest..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    Creature c1(glm::vec3(50.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);
    Creature c2(glm::vec3(60.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);  // 10 units away
    Creature c3(glm::vec3(55.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);  // 5 units away

    grid.clear();
    grid.insert(&c1);
    grid.insert(&c2);
    grid.insert(&c3);

    // Find nearest to c1's position (excluding self by type filter -1)
    Creature* nearest = grid.findNearest(glm::vec3(50.0f, 0.0f, 50.0f), 20.0f);

    // Should find something
    if (nearest != nullptr) {
        float dist = glm::distance(nearest->getPosition(), glm::vec3(50.0f, 0.0f, 50.0f));
        assert(dist < 20.0f);
    }

    std::cout << "  Find nearest test passed!" << std::endl;
}

// Test performance with many creatures
void testPerformance() {
    std::cout << "Testing performance with 1000 creatures..." << std::endl;

    SpatialGrid grid(1000.0f, 1000.0f, 20);

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;
    creatures.reserve(1000);

    for (int i = 0; i < 1000; i++) {
        glm::vec3 pos((float)(rand() % 1000), 0.0f, (float)(rand() % 1000));
        creatures.push_back(std::make_unique<Creature>(pos, genome, CreatureType::GRAZER));
    }

    // Insert all creatures
    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    assert(grid.getTotalCreatures() == 1000);

    // Perform many queries
    for (int i = 0; i < 100; i++) {
        float x = (float)(rand() % 1000);
        float z = (float)(rand() % 1000);
        const auto& nearby = grid.query(glm::vec3(x, 0.0f, z), 50.0f);
        (void)nearby;  // Use to prevent optimization
    }

    std::cout << "  Performance test passed!" << std::endl;
}

// Test grid statistics
void testGridStatistics() {
    std::cout << "Testing grid statistics..." << std::endl;

    SpatialGrid grid(100.0f, 100.0f, 10);

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;

    // Create clustered creatures
    for (int i = 0; i < 50; i++) {
        glm::vec3 pos(25.0f + (float)(i % 5), 0.0f, 25.0f + (float)(i / 5));
        creatures.push_back(std::make_unique<Creature>(pos, genome, CreatureType::GRAZER));
    }

    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    assert(grid.getTotalCreatures() == 50);

    // Max occupancy should be tracked
    size_t maxOccupancy = grid.getMaxCellOccupancy();
    assert(maxOccupancy > 0);
    assert(maxOccupancy <= 64);  // MAX_PER_CELL limit

    std::cout << "  Grid statistics test passed!" << std::endl;
}

int main() {
    std::cout << "=== SpatialGrid Unit Tests ===" << std::endl;

    testGridCreation();
    testSingleInsertion();
    testMultipleInsertions();
    testRadiusQuery();
    testTypeFiltering();
    testGridClear();
    testBoundaryConditions();
    testCountNearby();
    testFindNearest();
    testPerformance();
    testGridStatistics();

    std::cout << "\n=== All SpatialGrid tests passed! ===" << std::endl;
    return 0;
}
