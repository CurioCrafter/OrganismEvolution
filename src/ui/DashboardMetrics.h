#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

class Creature;
class Food;

namespace ai {
    class NeuralNetwork;
}

// History size for graphs (300 samples = 5 minutes at ~1 sample/second)
static constexpr int DASHBOARD_HISTORY_SIZE = 300;

// Structure to hold species information
struct SpeciesInfo {
    int id = 0;
    std::string name;
    int memberCount = 0;
    float avgFitness = 0.0f;
    float avgSize = 0.0f;
    float avgSpeed = 0.0f;
    glm::vec3 avgColor{0};
    int generation = 0;
    int parentSpeciesId = -1;  // For phylogeny
};

// Structure to hold a creature's inspectable data
struct CreatureInspectorData {
    int id = -1;
    std::string typeStr;
    int generation = 0;
    float health = 0;
    float energy = 0;
    float age = 0;
    float size = 0;
    float speed = 0;
    float visionRange = 0;
    float efficiency = 0;
    float metabolism = 0;
    glm::vec3 color{0};
    glm::vec3 position{0};
    float velocityMag = 0;
    float fitness = 0;
    int killCount = 0;
    float fear = 0;
    bool alive = false;
    int offspringCount = 0;
    int ancestorCount = 0;

    // Neural network info
    int brainNodeCount = 0;
    int brainConnectionCount = 0;
    float brainComplexity = 0.0f;
};

class DashboardMetrics {
public:
    DashboardMetrics();

    // Update metrics from simulation state
    void update(const std::vector<Creature*>& creatures,
                const std::vector<Food*>& food,
                float deltaTime);

    // Update frame time (call every frame)
    void recordFrameTime(float frameTimeMs);

    // Update memory usage
    void updateMemoryUsage();

    // Population history (for graphs)
    std::vector<float> herbivoreHistory;
    std::vector<float> carnivoreHistory;
    std::vector<float> totalPopHistory;
    std::vector<float> foodHistory;

    // Current population counts
    int herbivoreCount = 0;
    int carnivoreCount = 0;
    int totalCreatures = 0;
    int foodCount = 0;

    // Genetic diversity metrics
    float avgSize = 0, stdSize = 0;
    float avgSpeed = 0, stdSpeed = 0;
    float avgVision = 0, stdVision = 0;
    float avgEfficiency = 0, stdEfficiency = 0;
    float geneticDiversity = 0; // Combined metric 0-1

    // Ecosystem health indicators
    float predatorPreyRatio = 0;  // Ideal: 0.2-0.3
    float avgCreatureAge = 0;
    float avgCreatureEnergy = 0;
    int birthsThisMinute = 0;
    int deathsThisMinute = 0;
    float foodAvailabilityRatio = 0;  // food / herbivores
    float ecosystemHealth = 0;  // 0-100 health score

    // Generation tracking
    int maxGeneration = 0;
    float avgGeneration = 0;

    // Performance metrics
    float fps = 0;
    float updateTime = 0;  // ms per update

    // Frame time history for graph (circular buffer)
    static constexpr int FRAME_TIME_HISTORY_SIZE = 120;
    float frameTimeHistory[FRAME_TIME_HISTORY_SIZE] = {0};
    int frameTimeIndex = 0;
    float avgFrameTime = 0;
    float minFrameTime = 999.0f;
    float maxFrameTime = 0;

    // Memory usage (platform dependent)
    size_t memoryUsageMB = 0;

    // Simulation speed control
    float simulationSpeed = 1.0f;
    bool simulationPaused = false;

    // Selected creature for inspector
    Creature* selectedCreature = nullptr;
    CreatureInspectorData inspectorData;

    // Heat map data (20x20 grid)
    static constexpr int HEATMAP_SIZE = 20;
    float populationHeatmap[HEATMAP_SIZE][HEATMAP_SIZE] = {{0}};
    float foodHeatmap[HEATMAP_SIZE][HEATMAP_SIZE] = {{0}};

    // Species tracking
    std::vector<SpeciesInfo> speciesList;
    int totalSpeciesCount = 0;

    // Fitness history for graphs
    std::vector<float> fitnessHistory;

    // Aquatic and Flying counts
    int aquaticCount = 0;
    int flyingCount = 0;

    // Event tracking
    void recordBirth() { birthsThisMinute++; pendingBirths++; totalBirths++; }
    void recordDeath() { deathsThisMinute++; pendingDeaths++; totalDeaths++; }
    int totalBirths = 0;
    int totalDeaths = 0;

    // Selection
    void selectCreature(Creature* c);
    void clearSelection() { selectedCreature = nullptr; }
    bool hasSelection() const { return selectedCreature != nullptr; }

    // Get the neural network of selected creature (for visualization)
    const ai::NeuralNetwork* getSelectedBrain() const;

private:
    // Timing
    float minuteTimer = 0;
    float historyTimer = 0;
    int frameCounter = 0;

    // Pending events (reset each minute)
    int pendingBirths = 0;
    int pendingDeaths = 0;

    // History recording interval (1 second)
    static constexpr float HISTORY_INTERVAL = 1.0f;

    // Update functions
    void updatePopulationCounts(const std::vector<Creature*>& creatures);
    void updateGeneticDiversity(const std::vector<Creature*>& creatures);
    void updateEcosystemHealth(const std::vector<Creature*>& creatures,
                                const std::vector<Food*>& food);
    void updateHeatmaps(const std::vector<Creature*>& creatures,
                        const std::vector<Food*>& food);
    void updateInspectorData();
    void updateSpeciesInfo(const std::vector<Creature*>& creatures);
    void recordHistory();
    void trimHistory();
};
