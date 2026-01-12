#pragma once

#include "Genome.h"
#include "NeuralNetwork.h"
#include <glm/glm.hpp>
#include <memory>

class Terrain;

class Creature {
public:
    Creature(const glm::vec3& position, const Genome& genome);
    Creature(const glm::vec3& position, const Genome& parent1, const Genome& parent2);

    void update(float deltaTime, const Terrain& terrain, const std::vector<glm::vec3>& foodPositions, const std::vector<Creature*>& otherCreatures);
    void render(GLuint VAO);

    bool isAlive() const { return alive; }
    bool canReproduce() const { return energy > reproductionThreshold; }
    void consumeFood(float amount);
    void reproduce(float& energyCost);

    const glm::vec3& getPosition() const { return position; }
    const Genome& getGenome() const { return genome; }
    float getEnergy() const { return energy; }
    float getFitness() const { return fitness; }
    int getGeneration() const { return generation; }

    void setGeneration(int gen) { generation = gen; }

private:
    glm::vec3 position;
    glm::vec3 velocity;
    float rotation;

    Genome genome;
    std::unique_ptr<NeuralNetwork> brain;

    float energy;
    float age;
    bool alive;
    int generation;

    float fitness; // For evolutionary selection
    int foodEaten;
    float distanceTraveled;

    static constexpr float maxEnergy = 200.0f;
    static constexpr float reproductionThreshold = 150.0f;
    static constexpr float reproductionCost = 50.0f;

    void updatePhysics(float deltaTime, const Terrain& terrain);
    void updateBehavior(float deltaTime, const std::vector<glm::vec3>& foodPositions, const std::vector<Creature*>& otherCreatures);
    void calculateFitness();
};
