#include "Creature.h"
#include "../environment/Terrain.h"
#include <cmath>
#include <algorithm>

Creature::Creature(const glm::vec3& position, const Genome& genome)
    : position(position), velocity(0.0f), rotation(0.0f), genome(genome),
      energy(100.0f), age(0.0f), alive(true), generation(0),
      fitness(0.0f), foodEaten(0), distanceTraveled(0.0f) {

    brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);
}

Creature::Creature(const glm::vec3& position, const Genome& parent1, const Genome& parent2)
    : position(position), velocity(0.0f), rotation(0.0f), genome(parent1, parent2),
      energy(100.0f), age(0.0f), alive(true), generation(0),
      fitness(0.0f), foodEaten(0), distanceTraveled(0.0f) {

    brain = std::make_unique<NeuralNetwork>(genome.neuralWeights);
}

void Creature::update(float deltaTime, const Terrain& terrain,
                      const std::vector<glm::vec3>& foodPositions,
                      const std::vector<Creature*>& otherCreatures) {
    if (!alive) return;

    age += deltaTime;

    // Energy consumption (based on size and efficiency)
    float energyConsumption = (1.0f + genome.size * 0.5f) * genome.efficiency * deltaTime;
    energy -= energyConsumption;

    // Movement increases energy consumption
    float movementCost = glm::length(velocity) * 0.1f * deltaTime;
    energy -= movementCost;

    // Die if out of energy
    if (energy <= 0.0f) {
        alive = false;
        return;
    }

    // Update behavior using neural network
    updateBehavior(deltaTime, foodPositions, otherCreatures);

    // Update physics
    updatePhysics(deltaTime, terrain);

    // Calculate fitness
    calculateFitness();
}

void Creature::updateBehavior(float deltaTime, const std::vector<glm::vec3>& foodPositions,
                               const std::vector<Creature*>& otherCreatures) {
    // Find nearest food
    float nearestFoodDist = 1000.0f;
    glm::vec3 nearestFoodPos(0.0f);

    for (const auto& foodPos : foodPositions) {
        float dist = glm::length(foodPos - position);
        if (dist < nearestFoodDist) {
            nearestFoodDist = dist;
            nearestFoodPos = foodPos;
        }
    }

    // Calculate angle to nearest food
    float angleToFood = 0.0f;
    if (nearestFoodDist < 1000.0f) {
        glm::vec3 toFood = nearestFoodPos - position;
        angleToFood = atan2(toFood.z, toFood.x);
    }

    // Normalize inputs
    float normalizedFoodDist = std::min(nearestFoodDist / genome.visionRange, 1.0f);
    float normalizedAngle = angleToFood / 3.14159f;
    float normalizedEnergy = energy / maxEnergy;
    float threatDist = 1.0f; // No threats for now

    // Prepare inputs for neural network
    std::vector<float> inputs = {
        normalizedFoodDist,
        normalizedAngle,
        normalizedEnergy,
        threatDist
    };

    // Get output from neural network
    float desiredAngle, speedMultiplier;
    brain->process(inputs, desiredAngle, speedMultiplier);

    // Apply movement
    rotation = desiredAngle;
    float speed = genome.speed * speedMultiplier;

    velocity.x = cos(rotation) * speed;
    velocity.z = sin(rotation) * speed;
}

void Creature::updatePhysics(float deltaTime, const Terrain& terrain) {
    glm::vec3 oldPos = position;

    // Update position
    position += velocity * deltaTime;

    // Track distance traveled
    distanceTraveled += glm::length(position - oldPos);

    // Terrain boundaries
    float halfWidth = terrain.getWidth() * terrain.getScale() * 0.5f;
    float halfDepth = terrain.getDepth() * terrain.getScale() * 0.5f;

    position.x = std::clamp(position.x, -halfWidth, halfWidth);
    position.z = std::clamp(position.z, -halfDepth, halfDepth);

    // Stay on terrain (avoid water)
    if (terrain.isWater(position.x, position.z)) {
        position = oldPos; // Don't move into water
        velocity *= 0.5f; // Reduce velocity
    } else {
        position.y = terrain.getHeight(position.x, position.z) + genome.size;
    }
}

void Creature::consumeFood(float amount) {
    energy = std::min(energy + amount, maxEnergy);
    foodEaten++;
}

void Creature::reproduce(float& energyCost) {
    energy -= reproductionCost;
    energyCost = reproductionCost;
}

void Creature::calculateFitness() {
    // Fitness based on survival time, food eaten, and exploration
    fitness = age * 0.5f +
              foodEaten * 10.0f +
              distanceTraveled * 0.01f;
}

void Creature::render(GLuint VAO) {
    // Rendering handled by main simulation for now
    // Could render creature mesh here
}
