// test_integration.cpp - Integration tests for OrganismEvolution
// Tests complete workflows and system interactions

#include "entities/Creature.h"
#include "entities/Genome.h"
#include "entities/CreatureType.h"
#include "utils/SpatialGrid.h"
#include "core/Serializer.h"
#include <glm/glm.hpp>
#include <cassert>
#include <iostream>
#include <cmath>
#include <vector>
#include <memory>
#include <chrono>

// Helper function for float comparison
bool approxEqual(float a, float b, float epsilon = 0.01f) {
    return std::abs(a - b) < epsilon;
}

// Test basic creature creation
void testCreatureCreation() {
    std::cout << "Testing creature creation..." << std::endl;

    Genome genome;
    genome.randomize();

    Creature herbivore(glm::vec3(50.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);

    // Check basic properties
    assert(herbivore.isAlive());
    assert(herbivore.getType() == CreatureType::GRAZER);
    assert(herbivore.getEnergy() > 0.0f);

    // Check position
    glm::vec3 pos = herbivore.getPosition();
    assert(approxEqual(pos.x, 50.0f));
    assert(approxEqual(pos.z, 50.0f));

    std::cout << "  Creature creation test passed!" << std::endl;
}

// Test genetic inheritance
void testGeneticInheritance() {
    std::cout << "Testing genetic inheritance..." << std::endl;

    Genome parent1, parent2;
    parent1.randomize();
    parent2.randomize();

    // Force distinct values
    parent1.size = 1.0f;
    parent2.size = 2.0f;
    parent1.speed = 1.0f;
    parent2.speed = 3.0f;

    // Create 10 children and check inheritance
    for (int i = 0; i < 10; i++) {
        Genome child(parent1, parent2);

        // Size should be influenced by parents (blend or inherit)
        assert(child.size >= 0.5f && child.size <= 3.0f);
        assert(child.speed >= 0.5f && child.speed <= 4.0f);
    }

    std::cout << "  Genetic inheritance test passed!" << std::endl;
}

// Test spatial grid with creatures
void testSpatialGridIntegration() {
    std::cout << "Testing spatial grid integration..." << std::endl;

    SpatialGrid grid(200.0f, 200.0f, 20);

    Genome genome;
    genome.randomize();

    std::vector<std::unique_ptr<Creature>> creatures;

    // Create 100 creatures in random positions
    for (int i = 0; i < 100; i++) {
        CreatureType type = (i % 2 == 0) ? CreatureType::GRAZER : CreatureType::APEX_PREDATOR;
        glm::vec3 pos((float)(rand() % 200), 0.0f, (float)(rand() % 200));
        creatures.push_back(std::make_unique<Creature>(pos, genome, type));
    }

    // Insert into grid
    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    assert(grid.getTotalCreatures() == 100);

    // Test queries
    const auto& nearby = grid.query(glm::vec3(100.0f, 0.0f, 100.0f), 50.0f);

    // Should find some creatures
    assert(nearby.size() > 0);

    std::cout << "  Spatial grid integration test passed!" << std::endl;
}

// Test creature type traits
void testCreatureTypeTraits() {
    std::cout << "Testing creature type traits..." << std::endl;

    Genome genome;
    genome.randomize();

    // Test flying type
    Creature flyer(glm::vec3(0.0f, 0.0f, 0.0f), genome, CreatureType::FLYING_BIRD);
    assert(isFlying(flyer.getType()));
    assert(!isAquatic(flyer.getType()));

    // Test aquatic type
    Creature swimmer(glm::vec3(0.0f, 0.0f, 0.0f), genome, CreatureType::AQUATIC_PREDATOR);
    assert(isAquatic(swimmer.getType()));
    assert(!isFlying(swimmer.getType()));

    // Test herbivore type
    Creature herbivore(glm::vec3(0.0f, 0.0f, 0.0f), genome, CreatureType::GRAZER);
    assert(isHerbivore(herbivore.getType()));
    assert(!isPredator(herbivore.getType()));

    // Test predator type
    Creature predator(glm::vec3(0.0f, 0.0f, 0.0f), genome, CreatureType::APEX_PREDATOR);
    assert(isPredator(predator.getType()));
    assert(!isHerbivore(predator.getType()));

    std::cout << "  Creature type traits test passed!" << std::endl;
}

// Test save/load data structures
void testSaveLoadStructures() {
    std::cout << "Testing save/load data structures..." << std::endl;

    // Create creature save data
    Forge::CreatureSaveData saveData;
    saveData.posX = 50.0f;
    saveData.posY = 0.0f;
    saveData.posZ = 50.0f;
    saveData.velX = 1.0f;
    saveData.velY = 0.0f;
    saveData.velZ = 0.0f;
    saveData.rotation = 1.5f;
    saveData.health = 75.0f;
    saveData.energy = 60.0f;
    saveData.age = 120.0f;
    saveData.generation = 5;
    saveData.type = static_cast<uint8_t>(CreatureType::GRAZER);

    // Verify data is stored correctly
    assert(approxEqual(saveData.health, 75.0f));
    assert(approxEqual(saveData.energy, 60.0f));
    assert(saveData.generation == 5);

    // Create food save data
    Forge::FoodSaveData foodData;
    foodData.posX = 25.0f;
    foodData.posY = 0.0f;
    foodData.posZ = 25.0f;
    foodData.energy = 30.0f;
    foodData.active = true;
    foodData.respawnTimer = 0.0f;

    assert(approxEqual(foodData.energy, 30.0f));
    assert(foodData.active);

    std::cout << "  Save/load structures test passed!" << std::endl;
}

// Test animation initialization
void testAnimationInitialization() {
    std::cout << "Testing animation initialization..." << std::endl;

    Genome genome;
    genome.randomize();

    // Test each creature type initializes without crash
    std::vector<CreatureType> types = {
        CreatureType::GRAZER,
        CreatureType::BROWSER,
        CreatureType::APEX_PREDATOR,
        CreatureType::FLYING,
        CreatureType::FLYING_BIRD,
        CreatureType::AQUATIC,
        CreatureType::AQUATIC_PREDATOR
    };

    for (CreatureType type : types) {
        Creature c(glm::vec3(0.0f, 0.0f, 0.0f), genome, type);
        c.initializeAnimation();

        // Should not crash and should have animation state
        assert(c.isAnimationEnabled());
    }

    std::cout << "  Animation initialization test passed!" << std::endl;
}

// Test multiple creature types
void testMultipleCreatureTypes() {
    std::cout << "Testing multiple creature types..." << std::endl;

    Genome genome;
    genome.randomize();

    SpatialGrid grid(200.0f, 200.0f, 20);

    std::vector<std::unique_ptr<Creature>> creatures;

    // Create mixed population
    std::vector<CreatureType> types = {
        CreatureType::GRAZER,
        CreatureType::APEX_PREDATOR,
        CreatureType::FLYING,
        CreatureType::AQUATIC
    };

    for (int i = 0; i < 50; i++) {
        CreatureType type = types[i % types.size()];
        glm::vec3 pos((float)(rand() % 200), isFlying(type) ? 20.0f : 0.0f, (float)(rand() % 200));
        creatures.push_back(std::make_unique<Creature>(pos, genome, type));
    }

    grid.clear();
    for (auto& c : creatures) {
        grid.insert(c.get());
    }

    // All creatures should be in grid
    assert(grid.getTotalCreatures() == 50);

    // All creatures should have valid positions
    for (auto& c : creatures) {
        glm::vec3 pos = c->getPosition();
        assert(!std::isnan(pos.x));
        assert(!std::isnan(pos.z));
    }

    std::cout << "  Multiple creature types test passed!" << std::endl;
}

// Test genome evolution over generations
void testGenomeEvolution() {
    std::cout << "Testing genome evolution over generations..." << std::endl;

    std::vector<Genome> population;

    // Create initial population
    for (int i = 0; i < 20; i++) {
        Genome g;
        g.randomize();
        population.push_back(g);
    }

    // Track initial average size
    float initialAvgSize = 0.0f;
    for (auto& g : population) {
        initialAvgSize += g.size;
    }
    initialAvgSize /= population.size();

    // Simulate multiple generations
    for (int gen = 0; gen < 5; gen++) {
        std::vector<Genome> nextGen;

        // Reproduce
        for (size_t i = 0; i < population.size(); i += 2) {
            if (i + 1 < population.size()) {
                Genome child(population[i], population[i + 1]);
                child.mutate(0.1f);
                nextGen.push_back(child);
            }
        }

        // Add parents to next gen
        for (auto& g : population) {
            if (nextGen.size() < 20) {
                nextGen.push_back(g);
            }
        }

        population = std::move(nextGen);
    }

    // Population should still exist
    assert(!population.empty());

    // Calculate final average size
    float finalAvgSize = 0.0f;
    for (auto& g : population) {
        finalAvgSize += g.size;
    }
    finalAvgSize /= population.size();

    // Some genetic drift should have occurred
    std::cout << "  Initial avg size: " << initialAvgSize << std::endl;
    std::cout << "  Final avg size: " << finalAvgSize << std::endl;

    std::cout << "  Genome evolution test passed!" << std::endl;
}

// Test neural network behavior integration
void testNeuralBehaviorIntegration() {
    std::cout << "Testing neural network behavior integration..." << std::endl;

    Genome genome;
    genome.randomize();

    Creature creature(glm::vec3(50.0f, 0.0f, 50.0f), genome, CreatureType::GRAZER);

    // Creature should have neural network from genome weights
    assert(genome.neuralWeights.size() > 0);

    // Creating many creatures should work
    std::vector<std::unique_ptr<Creature>> creatures;
    for (int i = 0; i < 100; i++) {
        Genome g;
        g.randomize();
        glm::vec3 pos((float)(rand() % 100), 0.0f, (float)(rand() % 100));
        creatures.push_back(std::make_unique<Creature>(pos, g, CreatureType::GRAZER));
    }

    assert(creatures.size() == 100);

    std::cout << "  Neural behavior integration test passed!" << std::endl;
}

// Test diploid genome (new genetic system)
void testDiploidGenome() {
    std::cout << "Testing diploid genome..." << std::endl;

    Genome genome;
    genome.randomize();

    // Create creature with diploid genome
    Creature creature(glm::vec3(0.0f, 0.0f, 0.0f), genome, CreatureType::GRAZER);

    // Diploid genome should be accessible
    const auto& diploid = creature.getDiploidGenome();

    // Species ID should be set
    auto speciesId = diploid.getSpeciesId();
    (void)speciesId;  // Just verify it doesn't crash

    std::cout << "  Diploid genome test passed!" << std::endl;
}

int main() {
    std::cout << "=== Integration Tests ===" << std::endl;

    testCreatureCreation();
    testGeneticInheritance();
    testSpatialGridIntegration();
    testCreatureTypeTraits();
    testSaveLoadStructures();
    testAnimationInitialization();
    testMultipleCreatureTypes();
    testGenomeEvolution();
    testNeuralBehaviorIntegration();
    testDiploidGenome();

    std::cout << "\n=== All Integration tests passed! ===" << std::endl;
    return 0;
}
