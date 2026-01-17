#include "DashboardMetrics.h"
#include "../entities/Creature.h"
#include "../entities/NeuralNetwork.h"
#include "../environment/Food.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>

// Platform-specific includes for memory usage
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
// Undefine Windows min/max macros that conflict with std::min/std::max
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#elif defined(__linux__)
#include <fstream>
#include <string>
#elif defined(__APPLE__)
#include <mach/mach.h>
#endif

// World size for heatmap calculations
static constexpr float WORLD_SIZE = 300.0f;

DashboardMetrics::DashboardMetrics() {
    // Reserve space for history vectors to avoid reallocations
    herbivoreHistory.reserve(DASHBOARD_HISTORY_SIZE);
    carnivoreHistory.reserve(DASHBOARD_HISTORY_SIZE);
    totalPopHistory.reserve(DASHBOARD_HISTORY_SIZE);
    foodHistory.reserve(DASHBOARD_HISTORY_SIZE);
    fitnessHistory.reserve(DASHBOARD_HISTORY_SIZE);

    // Initialize heatmaps to zero
    for (int i = 0; i < HEATMAP_SIZE; ++i) {
        for (int j = 0; j < HEATMAP_SIZE; ++j) {
            populationHeatmap[i][j] = 0.0f;
            foodHeatmap[i][j] = 0.0f;
        }
    }
}

void DashboardMetrics::update(const std::vector<Creature*>& creatures,
                               const std::vector<Food*>& food,
                               float deltaTime) {
    // Increment frame counter for FPS calculation
    frameCounter++;

    // Update timers
    minuteTimer += deltaTime;
    historyTimer += deltaTime;

    // Calculate FPS (simple moving average approximation)
    if (deltaTime > 0.0f) {
        float instantFps = 1.0f / deltaTime;
        fps = fps * 0.95f + instantFps * 0.05f; // Smooth FPS
    }

    // Update all metrics
    updatePopulationCounts(creatures);
    updateGeneticDiversity(creatures);
    updateEcosystemHealth(creatures, food);
    updateHeatmaps(creatures, food);
    updateSpeciesInfo(creatures);

    // Update inspector if we have a selection
    if (selectedCreature != nullptr) {
        updateInspectorData();
    }

    // Record history every HISTORY_INTERVAL seconds
    if (historyTimer >= HISTORY_INTERVAL) {
        recordHistory();
        historyTimer = 0.0f;
    }

    // Reset minute counters every 60 seconds
    if (minuteTimer >= 60.0f) {
        birthsThisMinute = pendingBirths;
        deathsThisMinute = pendingDeaths;
        pendingBirths = 0;
        pendingDeaths = 0;
        minuteTimer = 0.0f;
    }
}

void DashboardMetrics::updatePopulationCounts(const std::vector<Creature*>& creatures) {
    herbivoreCount = 0;
    carnivoreCount = 0;
    aquaticCount = 0;
    flyingCount = 0;
    totalCreatures = 0;

    for (const Creature* creature : creatures) {
        if (creature == nullptr || !creature->isAlive()) {
            continue;
        }

        totalCreatures++;

        switch (creature->getType()) {
            case CreatureType::HERBIVORE:
                herbivoreCount++;
                break;
            case CreatureType::CARNIVORE:
                carnivoreCount++;
                break;
            case CreatureType::AQUATIC:
                aquaticCount++;
                break;
            case CreatureType::FLYING:
                flyingCount++;
                break;
        }
    }
}

void DashboardMetrics::updateGeneticDiversity(const std::vector<Creature*>& creatures) {
    if (creatures.empty()) {
        avgSize = avgSpeed = avgVision = avgEfficiency = 0.0f;
        stdSize = stdSpeed = stdVision = stdEfficiency = 0.0f;
        geneticDiversity = 0.0f;
        maxGeneration = 0;
        avgGeneration = 0.0f;
        return;
    }

    // Collect values from alive creatures
    std::vector<float> sizes, speeds, visions, efficiencies;
    std::vector<int> generations;

    sizes.reserve(creatures.size());
    speeds.reserve(creatures.size());
    visions.reserve(creatures.size());
    efficiencies.reserve(creatures.size());
    generations.reserve(creatures.size());

    for (const Creature* creature : creatures) {
        if (creature == nullptr || !creature->isAlive()) {
            continue;
        }

        const Genome& genome = creature->getGenome();
        sizes.push_back(genome.size);
        speeds.push_back(genome.speed);
        visions.push_back(genome.visionRange);
        efficiencies.push_back(genome.efficiency);
        generations.push_back(creature->getGeneration());
    }

    if (sizes.empty()) {
        avgSize = avgSpeed = avgVision = avgEfficiency = 0.0f;
        stdSize = stdSpeed = stdVision = stdEfficiency = 0.0f;
        geneticDiversity = 0.0f;
        maxGeneration = 0;
        avgGeneration = 0.0f;
        return;
    }

    int n = static_cast<int>(sizes.size());

    // Calculate means
    float sumSize = 0, sumSpeed = 0, sumVision = 0, sumEfficiency = 0;
    int sumGen = 0;

    for (int i = 0; i < n; ++i) {
        sumSize += sizes[i];
        sumSpeed += speeds[i];
        sumVision += visions[i];
        sumEfficiency += efficiencies[i];
        sumGen += generations[i];
    }

    avgSize = sumSize / n;
    avgSpeed = sumSpeed / n;
    avgVision = sumVision / n;
    avgEfficiency = sumEfficiency / n;
    avgGeneration = static_cast<float>(sumGen) / n;

    // Find max generation
    maxGeneration = 0;
    for (int gen : generations) {
        if (gen > maxGeneration) {
            maxGeneration = gen;
        }
    }

    // Calculate standard deviations
    float varSize = 0, varSpeed = 0, varVision = 0, varEfficiency = 0;

    for (int i = 0; i < n; ++i) {
        float diffSize = sizes[i] - avgSize;
        float diffSpeed = speeds[i] - avgSpeed;
        float diffVision = visions[i] - avgVision;
        float diffEfficiency = efficiencies[i] - avgEfficiency;

        varSize += diffSize * diffSize;
        varSpeed += diffSpeed * diffSpeed;
        varVision += diffVision * diffVision;
        varEfficiency += diffEfficiency * diffEfficiency;
    }

    if (n > 1) {
        varSize /= (n - 1);
        varSpeed /= (n - 1);
        varVision /= (n - 1);
        varEfficiency /= (n - 1);
    }

    stdSize = std::sqrt(varSize);
    stdSpeed = std::sqrt(varSpeed);
    stdVision = std::sqrt(varVision);
    stdEfficiency = std::sqrt(varEfficiency);

    // Calculate combined genetic diversity metric (0-1)
    // Normalize each std dev by expected range and average
    // Size range: 0.5-2.0, Speed range: 5-20, Vision range: 10-50, Efficiency range: 0.5-1.5
    float normSizeDiv = (avgSize > 0.0f) ? stdSize / avgSize : 0.0f;
    float normSpeedDiv = (avgSpeed > 0.0f) ? stdSpeed / avgSpeed : 0.0f;
    float normVisionDiv = (avgVision > 0.0f) ? stdVision / avgVision : 0.0f;
    float normEffDiv = (avgEfficiency > 0.0f) ? stdEfficiency / avgEfficiency : 0.0f;

    // Coefficient of variation based diversity (capped at 1.0)
    geneticDiversity = std::min(1.0f, (normSizeDiv + normSpeedDiv + normVisionDiv + normEffDiv) / 2.0f);
}

void DashboardMetrics::updateEcosystemHealth(const std::vector<Creature*>& creatures,
                                              const std::vector<Food*>& food) {
    // Count active food
    foodCount = 0;
    for (const Food* f : food) {
        if (f != nullptr && f->isActive()) {
            foodCount++;
        }
    }

    // Calculate predator-prey ratio
    if (herbivoreCount > 0) {
        predatorPreyRatio = static_cast<float>(carnivoreCount) / static_cast<float>(herbivoreCount);
    } else {
        predatorPreyRatio = (carnivoreCount > 0) ? 1.0f : 0.0f;
    }

    // Calculate food availability ratio
    if (herbivoreCount > 0) {
        foodAvailabilityRatio = static_cast<float>(foodCount) / static_cast<float>(herbivoreCount);
    } else {
        foodAvailabilityRatio = (foodCount > 0) ? 10.0f : 0.0f;
    }

    // Calculate average creature age and energy
    float totalAge = 0.0f;
    float totalEnergy = 0.0f;
    int aliveCount = 0;

    for (const Creature* creature : creatures) {
        if (creature == nullptr || !creature->isAlive()) {
            continue;
        }

        totalEnergy += creature->getEnergy();
        totalAge += creature->getAge();
        aliveCount++;
    }

    if (aliveCount > 0) {
        avgCreatureEnergy = totalEnergy / aliveCount;
        avgCreatureAge = totalAge / aliveCount;
    } else {
        avgCreatureEnergy = 0.0f;
        avgCreatureAge = 0.0f;
    }

    // Calculate ecosystem health score (0-100)
    // Factors:
    // 1. Population balance: Ideal predator-prey ratio around 0.2-0.3 (25 points)
    // 2. Genetic diversity: Higher is better (25 points)
    // 3. Food availability: Ideal ratio around 1.0-2.0 (25 points)
    // 4. Population stability: Births ≈ Deaths (25 points)

    float healthScore = 0.0f;

    // 1. Population balance (25 points)
    // Ideal ratio is 0.2-0.3, penalize deviation
    float idealRatioCenter = 0.25f;
    float ratioDiff = std::abs(predatorPreyRatio - idealRatioCenter);
    float ratioScore = std::max(0.0f, 25.0f - ratioDiff * 50.0f);
    healthScore += ratioScore;

    // 2. Genetic diversity (25 points)
    // Higher diversity is better (between 0.1 and 0.5 is healthy)
    float diversityScore = std::min(25.0f, geneticDiversity * 50.0f);
    healthScore += diversityScore;

    // 3. Food availability (25 points)
    // Ideal ratio around 1.0-2.0 food per herbivore
    float idealFoodRatio = 1.5f;
    float foodDiff = std::abs(foodAvailabilityRatio - idealFoodRatio);
    float foodScore = std::max(0.0f, 25.0f - foodDiff * 10.0f);
    healthScore += foodScore;

    // 4. Population stability (25 points)
    // Births should roughly equal deaths for stability
    int birthDeathDiff = std::abs(birthsThisMinute - deathsThisMinute);
    int totalEvents = birthsThisMinute + deathsThisMinute;
    float stabilityScore = 25.0f;
    if (totalEvents > 0) {
        float imbalance = static_cast<float>(birthDeathDiff) / static_cast<float>(totalEvents);
        stabilityScore = std::max(0.0f, 25.0f * (1.0f - imbalance));
    }
    healthScore += stabilityScore;

    // Bonus: Penalize extreme population sizes
    if (totalCreatures < 10) {
        healthScore *= 0.5f; // Low population penalty
    } else if (totalCreatures > 500) {
        healthScore *= 0.8f; // Overpopulation penalty
    }

    ecosystemHealth = std::clamp(healthScore, 0.0f, 100.0f);
}

void DashboardMetrics::updateHeatmaps(const std::vector<Creature*>& creatures,
                                       const std::vector<Food*>& food) {
    // Reset heatmaps
    for (int i = 0; i < HEATMAP_SIZE; ++i) {
        for (int j = 0; j < HEATMAP_SIZE; ++j) {
            populationHeatmap[i][j] = 0.0f;
            foodHeatmap[i][j] = 0.0f;
        }
    }

    float cellSize = WORLD_SIZE / static_cast<float>(HEATMAP_SIZE);
    float halfWorld = WORLD_SIZE / 2.0f;

    // Populate creature heatmap
    for (const Creature* creature : creatures) {
        if (creature == nullptr || !creature->isAlive()) {
            continue;
        }

        const glm::vec3& pos = creature->getPosition();

        // Convert world position to grid cell
        // World coords are roughly -150 to 150, convert to 0 to 300 first
        float normalizedX = pos.x + halfWorld;
        float normalizedZ = pos.z + halfWorld;

        int cellX = static_cast<int>(normalizedX / cellSize);
        int cellZ = static_cast<int>(normalizedZ / cellSize);

        // Clamp to valid range
        cellX = std::clamp(cellX, 0, HEATMAP_SIZE - 1);
        cellZ = std::clamp(cellZ, 0, HEATMAP_SIZE - 1);

        populationHeatmap[cellX][cellZ] += 1.0f;
    }

    // Populate food heatmap
    for (const Food* f : food) {
        if (f == nullptr || !f->isActive()) {
            continue;
        }

        const glm::vec3& pos = f->getPosition();

        float normalizedX = pos.x + halfWorld;
        float normalizedZ = pos.z + halfWorld;

        int cellX = static_cast<int>(normalizedX / cellSize);
        int cellZ = static_cast<int>(normalizedZ / cellSize);

        cellX = std::clamp(cellX, 0, HEATMAP_SIZE - 1);
        cellZ = std::clamp(cellZ, 0, HEATMAP_SIZE - 1);

        foodHeatmap[cellX][cellZ] += 1.0f;
    }

    // Normalize heatmaps (find max and scale)
    float maxPop = 1.0f;
    float maxFood = 1.0f;

    for (int i = 0; i < HEATMAP_SIZE; ++i) {
        for (int j = 0; j < HEATMAP_SIZE; ++j) {
            if (populationHeatmap[i][j] > maxPop) {
                maxPop = populationHeatmap[i][j];
            }
            if (foodHeatmap[i][j] > maxFood) {
                maxFood = foodHeatmap[i][j];
            }
        }
    }

    // Normalize to 0-1 range
    for (int i = 0; i < HEATMAP_SIZE; ++i) {
        for (int j = 0; j < HEATMAP_SIZE; ++j) {
            populationHeatmap[i][j] /= maxPop;
            foodHeatmap[i][j] /= maxFood;
        }
    }
}

void DashboardMetrics::updateInspectorData() {
    if (selectedCreature == nullptr) {
        inspectorData = CreatureInspectorData(); // Reset to defaults
        return;
    }

    // Check if creature is still alive
    if (!selectedCreature->isAlive()) {
        inspectorData.alive = false;
        // Keep other data for display but mark as dead
        return;
    }

    inspectorData.alive = true;
    inspectorData.id = selectedCreature->getID();

    // Set type string
    if (selectedCreature->getType() == CreatureType::HERBIVORE) {
        inspectorData.typeStr = "Herbivore";
    } else if (selectedCreature->getType() == CreatureType::CARNIVORE) {
        inspectorData.typeStr = "Carnivore";
    } else {
        inspectorData.typeStr = "Unknown";
    }

    inspectorData.generation = selectedCreature->getGeneration();
    inspectorData.energy = selectedCreature->getEnergy();
    inspectorData.health = inspectorData.energy; // Use energy as health proxy

    // Get genome data
    const Genome& genome = selectedCreature->getGenome();
    inspectorData.size = genome.size;
    inspectorData.speed = genome.speed;
    inspectorData.visionRange = genome.visionRange;
    inspectorData.color = genome.color;

    // Position and velocity
    inspectorData.position = selectedCreature->getPosition();
    inspectorData.velocityMag = glm::length(selectedCreature->getVelocity());

    // Other stats
    inspectorData.fitness = selectedCreature->getFitness();
    inspectorData.killCount = selectedCreature->getKillCount();
    inspectorData.fear = selectedCreature->getFear();

    // Age from creature
    inspectorData.age = selectedCreature->getAge();
}

void DashboardMetrics::recordHistory() {
    // Push current values to history vectors
    herbivoreHistory.push_back(static_cast<float>(herbivoreCount));
    carnivoreHistory.push_back(static_cast<float>(carnivoreCount));
    totalPopHistory.push_back(static_cast<float>(totalCreatures));
    foodHistory.push_back(static_cast<float>(foodCount));

    // Calculate and record average fitness
    float totalFitness = 0.0f;
    int count = 0;
    // Note: We don't have direct access to creatures here, so use ecosystemHealth as proxy
    fitnessHistory.push_back(ecosystemHealth / 100.0f);

    // Trim if necessary
    trimHistory();
}

void DashboardMetrics::trimHistory() {
    // Keep vectors at DASHBOARD_HISTORY_SIZE max
    while (herbivoreHistory.size() > DASHBOARD_HISTORY_SIZE) {
        herbivoreHistory.erase(herbivoreHistory.begin());
    }
    while (carnivoreHistory.size() > DASHBOARD_HISTORY_SIZE) {
        carnivoreHistory.erase(carnivoreHistory.begin());
    }
    while (totalPopHistory.size() > DASHBOARD_HISTORY_SIZE) {
        totalPopHistory.erase(totalPopHistory.begin());
    }
    while (foodHistory.size() > DASHBOARD_HISTORY_SIZE) {
        foodHistory.erase(foodHistory.begin());
    }
    while (fitnessHistory.size() > DASHBOARD_HISTORY_SIZE) {
        fitnessHistory.erase(fitnessHistory.begin());
    }
}

void DashboardMetrics::selectCreature(Creature* c) {
    selectedCreature = c;
    if (c != nullptr) {
        updateInspectorData();
    } else {
        inspectorData = CreatureInspectorData(); // Reset to defaults
    }
}

void DashboardMetrics::recordFrameTime(float frameTimeMs) {
    // Store in circular buffer
    frameTimeHistory[frameTimeIndex] = frameTimeMs;
    frameTimeIndex = (frameTimeIndex + 1) % FRAME_TIME_HISTORY_SIZE;

    // Calculate statistics
    float sum = 0.0f;
    minFrameTime = 999.0f;
    maxFrameTime = 0.0f;

    for (int i = 0; i < FRAME_TIME_HISTORY_SIZE; ++i) {
        float time = frameTimeHistory[i];
        if (time > 0.0f) {  // Only count non-zero entries
            sum += time;
            if (time < minFrameTime) minFrameTime = time;
            if (time > maxFrameTime) maxFrameTime = time;
        }
    }

    // Calculate average (avoid division by zero)
    int validSamples = 0;
    for (int i = 0; i < FRAME_TIME_HISTORY_SIZE; ++i) {
        if (frameTimeHistory[i] > 0.0f) validSamples++;
    }
    avgFrameTime = (validSamples > 0) ? sum / validSamples : 0.0f;
}

void DashboardMetrics::updateMemoryUsage() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        memoryUsageMB = pmc.WorkingSetSize / (1024 * 1024);
    }
#elif defined(__linux__)
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        size_t size, resident;
        statm >> size >> resident;
        // resident is in pages, typically 4KB each
        memoryUsageMB = (resident * 4096) / (1024 * 1024);
        statm.close();
    }
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t size = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &size) == KERN_SUCCESS) {
        memoryUsageMB = info.resident_size / (1024 * 1024);
    }
#else
    memoryUsageMB = 0; // Unsupported platform
#endif
}

void DashboardMetrics::updateSpeciesInfo(const std::vector<Creature*>& creatures) {
    // Group creatures by type for basic species tracking
    // In a more sophisticated implementation, this would use genetic clustering

    speciesList.clear();

    // Track stats per creature type
    struct TypeStats {
        int count = 0;
        float totalFitness = 0.0f;
        float totalSize = 0.0f;
        float totalSpeed = 0.0f;
        glm::vec3 totalColor{0.0f};
        int maxGen = 0;
    };

    std::unordered_map<int, TypeStats> typeStats;

    for (const Creature* creature : creatures) {
        if (creature == nullptr || !creature->isAlive()) {
            continue;
        }

        int typeId = static_cast<int>(creature->getType());
        TypeStats& stats = typeStats[typeId];

        stats.count++;
        stats.totalFitness += creature->getFitness();
        stats.totalSize += creature->getGenome().size;
        stats.totalSpeed += creature->getGenome().speed;
        stats.totalColor += creature->getGenome().color;
        if (creature->getGeneration() > stats.maxGen) {
            stats.maxGen = creature->getGeneration();
        }
    }

    // Convert to SpeciesInfo structs
    static const char* typeNames[] = {"Herbivores", "Carnivores", "Aquatic", "Flying"};

    for (auto& [typeId, stats] : typeStats) {
        if (stats.count > 0) {
            SpeciesInfo info;
            info.id = typeId;
            info.name = (typeId >= 0 && typeId < 4) ? typeNames[typeId] : "Unknown";
            info.memberCount = stats.count;
            info.avgFitness = stats.totalFitness / stats.count;
            info.avgSize = stats.totalSize / stats.count;
            info.avgSpeed = stats.totalSpeed / stats.count;
            info.avgColor = stats.totalColor / static_cast<float>(stats.count);
            info.generation = stats.maxGen;
            speciesList.push_back(info);
        }
    }

    totalSpeciesCount = static_cast<int>(speciesList.size());
}

const ai::NeuralNetwork* DashboardMetrics::getSelectedBrain() const {
    // Note: This would require Creature to expose a getBrain() method
    // For now, return nullptr as the brain is private in Creature
    // This should be updated when Creature exposes brain access
    return nullptr;
}
