#pragma once

#include "../entities/Creature.h"
#include "../environment/Terrain.h"
#include "../environment/Food.h"
#include "../graphics/Shader.h"
#include "../graphics/Camera.h"
#include <vector>
#include <memory>

class Simulation {
public:
    Simulation();
    ~Simulation();

    void init();
    void update(float deltaTime);
    void render(Camera& camera);

    void togglePause() { paused = !paused; }
    void increaseSpeed() { simulationSpeed = std::min(simulationSpeed * 2.0f, 8.0f); }
    void decreaseSpeed() { simulationSpeed = std::max(simulationSpeed * 0.5f, 0.25f); }

    // Statistics
    int getPopulation() const { return creatures.size(); }
    int getGeneration() const { return generation; }
    float getAverageFitness() const;

private:
    std::unique_ptr<Terrain> terrain;
    std::vector<std::unique_ptr<Creature>> creatures;
    std::vector<std::unique_ptr<Food>> food;

    GLuint creatureVAO, creatureVBO;
    GLuint foodVAO, foodVBO;

    bool paused;
    float simulationSpeed;
    int generation;
    float timeSinceLastFood;
    float foodSpawnInterval;
    int maxFoodCount;

    // Evolution parameters
    float mutationRate;
    float mutationStrength;

    void spawnInitialPopulation();
    void spawnFood();
    void updateCreatures(float deltaTime);
    void updateFood(float deltaTime);
    void handleReproduction();
    void handleEvolution();
    void setupCreatureMesh();
    void setupFoodMesh();

    std::vector<glm::vec3> getFoodPositions() const;
};
