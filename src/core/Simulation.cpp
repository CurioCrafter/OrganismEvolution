#include "Simulation.h"
#include "../utils/Random.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

Simulation::Simulation()
    : paused(false), simulationSpeed(1.0f), generation(1),
      timeSinceLastFood(0.0f), foodSpawnInterval(0.2f), maxFoodCount(150),
      mutationRate(0.1f), mutationStrength(0.15f),
      creatureVAO(0), creatureVBO(0), foodVAO(0), foodVBO(0) {
}

Simulation::~Simulation() {
    glDeleteVertexArrays(1, &creatureVAO);
    glDeleteBuffers(1, &creatureVBO);
    glDeleteVertexArrays(1, &foodVAO);
    glDeleteBuffers(1, &foodVBO);
}

void Simulation::init() {
    Random::init();

    std::cout << "Generating terrain..." << std::endl;
    terrain = std::make_unique<Terrain>(150, 150, 2.0f);
    terrain->generate(12345);

    std::cout << "Creating initial population..." << std::endl;
    spawnInitialPopulation();

    std::cout << "Spawning initial food..." << std::endl;
    for (int i = 0; i < 100; i++) {
        spawnFood();
    }

    setupCreatureMesh();
    setupFoodMesh();

    std::cout << "Simulation initialized! Population: " << creatures.size() << std::endl;
}

void Simulation::update(float deltaTime) {
    if (paused) return;

    float adjustedDelta = deltaTime * simulationSpeed;

    // Update creatures
    updateCreatures(adjustedDelta);

    // Update food
    updateFood(adjustedDelta);

    // Handle reproduction
    handleReproduction();

    // Handle evolution (when population gets too small)
    handleEvolution();

    // Spawn new food
    timeSinceLastFood += adjustedDelta;
    if (timeSinceLastFood > foodSpawnInterval && food.size() < maxFoodCount) {
        spawnFood();
        timeSinceLastFood = 0.0f;
    }
}

void Simulation::render(Camera& camera) {
    // Render terrain
    terrain->render();

    // Render creatures (simple spheres for now)
    for (const auto& creature : creatures) {
        if (creature->isAlive()) {
            // TODO: Render creature meshes
        }
    }

    // Render food (simple cubes for now)
    for (const auto& foodItem : food) {
        if (foodItem->isActive()) {
            // TODO: Render food meshes
        }
    }
}

void Simulation::spawnInitialPopulation() {
    for (int i = 0; i < 50; i++) {
        Genome genome;
        genome.randomize();

        // Random position on land
        glm::vec3 position;
        do {
            position.x = Random::range(-100.0f, 100.0f);
            position.z = Random::range(-100.0f, 100.0f);
        } while (terrain->isWater(position.x, position.z));

        position.y = terrain->getHeight(position.x, position.z);

        auto creature = std::make_unique<Creature>(position, genome);
        creature->setGeneration(1);
        creatures.push_back(std::move(creature));
    }
}

void Simulation::spawnFood() {
    glm::vec3 position;
    int attempts = 0;
    do {
        position.x = Random::range(-100.0f, 100.0f);
        position.z = Random::range(-100.0f, 100.0f);
        attempts++;
    } while (terrain->isWater(position.x, position.z) && attempts < 10);

    if (attempts < 10) {
        position.y = terrain->getHeight(position.x, position.z) + 1.0f;
        food.push_back(std::make_unique<Food>(position));
    }
}

void Simulation::updateCreatures(float deltaTime) {
    std::vector<glm::vec3> foodPositions = getFoodPositions();

    // Create vector of creature pointers for creature-creature interaction
    std::vector<Creature*> creaturePtrs;
    for (auto& creature : creatures) {
        if (creature->isAlive()) {
            creaturePtrs.push_back(creature.get());
        }
    }

    // Update each creature
    for (auto& creature : creatures) {
        if (creature->isAlive()) {
            creature->update(deltaTime, *terrain, foodPositions, creaturePtrs);

            // Check for food collision
            for (auto& foodItem : food) {
                if (foodItem->isActive()) {
                    float dist = glm::length(foodItem->getPosition() - creature->getPosition());
                    if (dist < 2.0f) {
                        creature->consumeFood(40.0f);
                        foodItem->consume();
                    }
                }
            }
        }
    }

    // Remove dead creatures
    creatures.erase(
        std::remove_if(creatures.begin(), creatures.end(),
            [](const std::unique_ptr<Creature>& c) { return !c->isAlive(); }),
        creatures.end()
    );

    // Remove consumed food
    food.erase(
        std::remove_if(food.begin(), food.end(),
            [](const std::unique_ptr<Food>& f) { return !f->isActive(); }),
        food.end()
    );
}

void Simulation::updateFood(float deltaTime) {
    // Food doesn't need updating currently
}

void Simulation::handleReproduction() {
    std::vector<std::unique_ptr<Creature>> offspring;

    for (auto& creature : creatures) {
        if (creature->canReproduce()) {
            // Create offspring
            glm::vec3 childPos = creature->getPosition();
            childPos.x += Random::range(-5.0f, 5.0f);
            childPos.z += Random::range(-5.0f, 5.0f);

            // Self-reproduction with mutation
            Genome childGenome = creature->getGenome();
            childGenome.mutate(mutationRate, mutationStrength);

            auto child = std::make_unique<Creature>(childPos, childGenome);
            child->setGeneration(creature->getGeneration());

            float energyCost;
            creature->reproduce(energyCost);

            offspring.push_back(std::move(child));
        }
    }

    // Add offspring to population
    for (auto& child : offspring) {
        creatures.push_back(std::move(child));
    }
}

void Simulation::handleEvolution() {
    // If population is getting low, introduce some new random creatures
    if (creatures.size() < 10) {
        std::cout << "Population low! Adding new organisms. Generation: " << generation << std::endl;

        for (int i = 0; i < 20; i++) {
            Genome genome;
            genome.randomize();

            glm::vec3 position;
            do {
                position.x = Random::range(-100.0f, 100.0f);
                position.z = Random::range(-100.0f, 100.0f);
            } while (terrain->isWater(position.x, position.z));

            position.y = terrain->getHeight(position.x, position.z);

            auto creature = std::make_unique<Creature>(position, genome);
            creature->setGeneration(generation + 1);
            creatures.push_back(std::move(creature));
        }

        generation++;
    }

    // Cap population to prevent explosion
    if (creatures.size() > 200) {
        // Remove lowest fitness creatures
        std::sort(creatures.begin(), creatures.end(),
            [](const std::unique_ptr<Creature>& a, const std::unique_ptr<Creature>& b) {
                return a->getFitness() > b->getFitness();
            });

        creatures.resize(150);
    }
}

void Simulation::setupCreatureMesh() {
    // Simple sphere representation for creatures
    std::vector<float> vertices;

    // Create a simple octahedron (will look like a diamond)
    vertices = {
        0.0f, 1.0f, 0.0f,    0.0f, 1.0f, 0.0f,   // Top
        1.0f, 0.0f, 0.0f,    0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,    0.0f, 1.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,   0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, -1.0f,   0.0f, 1.0f, 0.0f,
        0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,  // Bottom
    };

    glGenVertexArrays(1, &creatureVAO);
    glGenBuffers(1, &creatureVBO);

    glBindVertexArray(creatureVAO);
    glBindBuffer(GL_ARRAY_BUFFER, creatureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Simulation::setupFoodMesh() {
    // Simple cube for food
    std::vector<float> vertices = {
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 0.0f,
    };

    glGenVertexArrays(1, &foodVAO);
    glGenBuffers(1, &foodVBO);

    glBindVertexArray(foodVAO);
    glBindBuffer(GL_ARRAY_BUFFER, foodVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

std::vector<glm::vec3> Simulation::getFoodPositions() const {
    std::vector<glm::vec3> positions;
    for (const auto& foodItem : food) {
        if (foodItem->isActive()) {
            positions.push_back(foodItem->getPosition());
        }
    }
    return positions;
}

float Simulation::getAverageFitness() const {
    if (creatures.empty()) return 0.0f;

    float totalFitness = 0.0f;
    for (const auto& creature : creatures) {
        totalFitness += creature->getFitness();
    }
    return totalFitness / creatures.size();
}
