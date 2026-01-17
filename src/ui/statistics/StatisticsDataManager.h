#pragma once

/**
 * @file StatisticsDataManager.h
 * @brief Centralized data collection and management for statistics visualization
 *
 * Collects data from various simulation systems and maintains rolling history
 * for graph rendering. Implements efficient sampling to avoid performance impact.
 */

#include "imgui.h"
#include "../../core/SimulationOrchestrator.h"
#include "../../core/CreatureManager.h"
#include "../../core/FoodChainManager.h"
#include "../../core/PerformanceManager.h"
#include "../../environment/EcosystemMetrics.h"
#include "../../entities/genetics/Species.h"
#include "../../entities/genetics/EvolutionaryHistory.h"
#include "../../entities/genetics/NicheSystem.h"
#include "../../entities/genetics/SelectionPressures.h"
#include "../../entities/Genome.h"
#include <glm/glm.hpp>
#include <vector>
#include <deque>
#include <map>
#include <array>
#include <string>
#include <cmath>

namespace ui {
namespace statistics {

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Population sample at a point in time
 */
struct PopulationSample {
    float time = 0.0f;
    int totalCreatures = 0;
    int herbivoreCount = 0;
    int carnivoreCount = 0;
    int omnivoreCount = 0;
    int aquaticCount = 0;
    int flyingCount = 0;
    int foodCount = 0;
    int speciesCount = 0;

    // Per-species populations
    std::map<genetics::SpeciesId, int> speciesPopulations;
};

/**
 * @brief Fitness metrics at a point in time
 */
struct FitnessSample {
    float time = 0.0f;
    float avgFitness = 0.0f;
    float maxFitness = 0.0f;
    float minFitness = 0.0f;
    float fitnessVariance = 0.0f;
    float geneticDiversity = 0.0f;
};

/**
 * @brief Statistics for a single trait
 */
struct TraitStatistics {
    float mean = 0.0f;
    float stdDev = 0.0f;
    float min = 0.0f;
    float max = 0.0f;
    float median = 0.0f;
    float skewness = 0.0f;
    std::vector<float> samples;
    std::vector<int> histogram;
    static constexpr int HISTOGRAM_BINS = 20;

    void calculate();
    void buildHistogram(float binMin, float binMax);
};

/**
 * @brief Collection of trait distributions
 */
struct TraitDistributions {
    TraitStatistics size;
    TraitStatistics speed;
    TraitStatistics visionRange;
    TraitStatistics efficiency;
    TraitStatistics aggression;
    TraitStatistics reproductionRate;
    TraitStatistics lifespan;
    TraitStatistics mutationRate;

    // Trait correlations matrix
    std::array<std::array<float, 8>, 8> correlations;

    void calculateAll();
    void calculateCorrelations();
};

/**
 * @brief Energy flow snapshot
 */
struct EnergyFlowSample {
    float time = 0.0f;
    float producerEnergy = 0.0f;
    float herbivoreEnergy = 0.0f;
    float carnivoreEnergy = 0.0f;
    float decomposerEnergy = 0.0f;
    float transferEfficiency = 0.0f;
};

/**
 * @brief Selection pressure snapshot
 */
struct SelectionPressureSample {
    float time = 0.0f;
    float predationPressure = 0.0f;
    float competitionPressure = 0.0f;
    float climatePressure = 0.0f;
    float foodPressure = 0.0f;
    float diseasePressure = 0.0f;
    float sexualSelectionPressure = 0.0f;
};

/**
 * @brief Niche occupancy snapshot
 */
struct NicheOccupancySample {
    float time = 0.0f;
    std::map<genetics::NicheType, int> occupancy;
    int occupiedNiches = 0;
    int emptyNiches = 0;
    float nicheOverlapIndex = 0.0f;
};

/**
 * @brief Speciation event record for display
 */
struct SpeciationEventDisplay {
    float time = 0.0f;
    genetics::SpeciesId parentId = 0;
    genetics::SpeciesId childId = 0;
    std::string parentName;
    std::string childName;
    genetics::SpeciationCause cause;
    glm::vec3 color;
};

/**
 * @brief Extinction event record for display
 */
struct ExtinctionEventDisplay {
    float time = 0.0f;
    genetics::SpeciesId speciesId = 0;
    std::string speciesName;
    genetics::ExtinctionCause cause;
    int finalPopulation = 0;
    int lifespan = 0;
};

// ============================================================================
// Statistics Data Manager
// ============================================================================

/**
 * @class StatisticsDataManager
 * @brief Central hub for collecting and managing simulation statistics
 *
 * Collects data from CreatureManager, EcosystemMetrics, Species tracker,
 * EvolutionaryHistory, NicheSystem, FoodChainManager, and SelectionPressures.
 * Maintains rolling history for time-series visualization.
 */
class StatisticsDataManager {
public:
    // Configuration
    static constexpr int MAX_HISTORY_POINTS = 1000;
    static constexpr float SAMPLE_INTERVAL = 0.5f;
    static constexpr float FAST_SAMPLE_INTERVAL = 0.1f;

    StatisticsDataManager();
    ~StatisticsDataManager() = default;

    // ========================================================================
    // Update
    // ========================================================================

    /**
     * @brief Update statistics from simulation systems
     * @param deltaTime Frame delta time
     * @param orchestrator Reference to main simulation orchestrator
     */
    void update(float deltaTime, SimulationOrchestrator& orchestrator);

    /**
     * @brief Update with individual system pointers (for flexibility)
     */
    void update(float deltaTime,
                const Forge::CreatureManager* creatures,
                const EcosystemMetrics* ecosystemMetrics,
                const genetics::SpeciationTracker* speciesTracker,
                const genetics::EvolutionaryHistoryTracker* evolutionHistory,
                const genetics::NicheManager* nicheManager,
                const Forge::FoodChainManager* foodChain,
                const genetics::SelectionPressureCalculator* selectionPressures,
                const Forge::PerformanceManager* performance,
                float simulationTime,
                int generation);

    // ========================================================================
    // Population Data Access
    // ========================================================================

    const std::deque<PopulationSample>& getPopulationHistory() const { return m_populationHistory; }
    const PopulationSample& getCurrentPopulation() const;

    // ========================================================================
    // Fitness Data Access
    // ========================================================================

    const std::deque<FitnessSample>& getFitnessHistory() const { return m_fitnessHistory; }
    const FitnessSample& getCurrentFitness() const;

    // ========================================================================
    // Trait Distribution Access
    // ========================================================================

    const TraitDistributions& getTraitDistributions() const { return m_traitDistributions; }

    // ========================================================================
    // Energy Flow Access
    // ========================================================================

    const std::deque<EnergyFlowSample>& getEnergyFlowHistory() const { return m_energyFlowHistory; }
    const EnergyFlowSample& getCurrentEnergyFlow() const;

    // ========================================================================
    // Selection Pressure Access
    // ========================================================================

    const std::deque<SelectionPressureSample>& getSelectionPressureHistory() const {
        return m_selectionPressureHistory;
    }
    const SelectionPressureSample& getCurrentSelectionPressures() const;

    // ========================================================================
    // Niche Occupancy Access
    // ========================================================================

    const std::deque<NicheOccupancySample>& getNicheOccupancyHistory() const {
        return m_nicheOccupancyHistory;
    }
    const NicheOccupancySample& getCurrentNicheOccupancy() const;

    // ========================================================================
    // Evolutionary Events Access
    // ========================================================================

    const std::deque<SpeciationEventDisplay>& getSpeciationEvents() const { return m_speciationEvents; }
    const std::deque<ExtinctionEventDisplay>& getExtinctionEvents() const { return m_extinctionEvents; }

    // ========================================================================
    // Performance Metrics Access
    // ========================================================================

    float getCurrentFPS() const { return m_currentFPS; }
    float getAverageFPS() const { return m_averageFPS; }
    const std::deque<float>& getFPSHistory() const { return m_fpsHistory; }
    int getDrawCalls() const { return m_drawCalls; }
    int getVisibleCreatures() const { return m_visibleCreatures; }
    size_t getMemoryUsage() const { return m_memoryUsage; }

    // ========================================================================
    // Summary Statistics
    // ========================================================================

    float getSpeciesDiversity() const { return m_speciesDiversity; }
    float getEcosystemHealth() const { return m_ecosystemHealth; }
    float getTrophicBalance() const { return m_trophicBalance; }
    int getTotalGenerations() const { return m_totalGenerations; }
    float getSimulationTime() const { return m_simulationTime; }

    // ========================================================================
    // Aquatic Ecosystem Statistics
    // ========================================================================

    /**
     * @brief Get aquatic creature counts by depth band
     * @return Array of counts: [Surface, Shallow, MidWater, Deep, Abyss]
     */
    const std::array<int, 5>& getAquaticDepthCounts() const { return m_aquaticDepthCounts; }

    /**
     * @brief Get total aquatic creature count
     */
    int getTotalAquaticCount() const;

    // ========================================================================
    // Control
    // ========================================================================

    void clear();
    void setSampleInterval(float interval) { m_sampleInterval = interval; }
    void setPaused(bool paused) { m_paused = paused; }
    bool isPaused() const { return m_paused; }

private:
    // Timing
    float m_timeSinceLastSample = 0.0f;
    float m_timeSinceFastSample = 0.0f;
    float m_sampleInterval = SAMPLE_INTERVAL;
    float m_simulationTime = 0.0f;
    int m_totalGenerations = 0;
    bool m_paused = false;

    // Population history
    std::deque<PopulationSample> m_populationHistory;
    PopulationSample m_currentPopulation;

    // Fitness history
    std::deque<FitnessSample> m_fitnessHistory;
    FitnessSample m_currentFitness;

    // Trait distributions (updated less frequently)
    TraitDistributions m_traitDistributions;
    float m_timeSinceTraitUpdate = 0.0f;
    static constexpr float TRAIT_UPDATE_INTERVAL = 2.0f;

    // Energy flow history
    std::deque<EnergyFlowSample> m_energyFlowHistory;
    EnergyFlowSample m_currentEnergyFlow;

    // Selection pressure history
    std::deque<SelectionPressureSample> m_selectionPressureHistory;
    SelectionPressureSample m_currentSelectionPressures;

    // Niche occupancy history
    std::deque<NicheOccupancySample> m_nicheOccupancyHistory;
    NicheOccupancySample m_currentNicheOccupancy;

    // Evolutionary events
    std::deque<SpeciationEventDisplay> m_speciationEvents;
    std::deque<ExtinctionEventDisplay> m_extinctionEvents;
    static constexpr size_t MAX_EVENTS = 100;

    // Performance metrics
    float m_currentFPS = 0.0f;
    float m_averageFPS = 0.0f;
    std::deque<float> m_fpsHistory;
    int m_drawCalls = 0;
    int m_visibleCreatures = 0;
    size_t m_memoryUsage = 0;

    // Summary statistics
    float m_speciesDiversity = 0.0f;
    float m_ecosystemHealth = 0.0f;
    float m_trophicBalance = 0.0f;

    // Aquatic ecosystem statistics (depth bands: Surface, Shallow, MidWater, Deep, Abyss)
    std::array<int, 5> m_aquaticDepthCounts = {0, 0, 0, 0, 0};

    // Last recorded speciation/extinction counts (for detecting new events)
    int m_lastSpeciationCount = 0;
    int m_lastExtinctionCount = 0;

    // Helper methods
    void samplePopulation(const Forge::CreatureManager* creatures,
                         const genetics::SpeciationTracker* speciesTracker);
    void sampleFitness(const Forge::CreatureManager* creatures);
    void sampleTraitDistributions(const Forge::CreatureManager* creatures);
    void sampleEnergyFlow(const Forge::FoodChainManager* foodChain,
                         const EcosystemMetrics* ecosystem);
    void sampleSelectionPressures(const genetics::SelectionPressureCalculator* pressures);
    void sampleNicheOccupancy(const genetics::NicheManager* nicheManager);
    void samplePerformance(const Forge::PerformanceManager* performance);
    void checkForNewEvents(const genetics::SpeciationTracker* tracker,
                          const genetics::EvolutionaryHistoryTracker* history);

    void trimHistory();
    void calculateSummaryStatistics(const EcosystemMetrics* ecosystem);
    void sampleAquaticDepths(const Forge::CreatureManager* creatures);
};

inline int StatisticsDataManager::getTotalAquaticCount() const {
    int total = 0;
    for (int count : m_aquaticDepthCounts) {
        total += count;
    }
    return total;
}

// ============================================================================
// Inline Implementations
// ============================================================================

inline void TraitStatistics::calculate() {
    if (samples.empty()) {
        mean = stdDev = min = max = median = skewness = 0.0f;
        return;
    }

    // Sort for median and percentiles
    std::vector<float> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    min = sorted.front();
    max = sorted.back();
    median = sorted[sorted.size() / 2];

    // Calculate mean
    float sum = 0.0f;
    for (float v : samples) {
        sum += v;
    }
    mean = sum / static_cast<float>(samples.size());

    // Calculate std dev and skewness
    float variance = 0.0f;
    float cubedSum = 0.0f;
    for (float v : samples) {
        float diff = v - mean;
        variance += diff * diff;
        cubedSum += diff * diff * diff;
    }
    variance /= static_cast<float>(samples.size());
    stdDev = std::sqrt(variance);

    // Skewness
    if (stdDev > 0.0001f) {
        skewness = cubedSum / (static_cast<float>(samples.size()) * stdDev * stdDev * stdDev);
    } else {
        skewness = 0.0f;
    }
}

inline void TraitStatistics::buildHistogram(float binMin, float binMax) {
    histogram.clear();
    histogram.resize(HISTOGRAM_BINS, 0);

    if (samples.empty() || binMax <= binMin) return;

    float binWidth = (binMax - binMin) / static_cast<float>(HISTOGRAM_BINS);

    for (float v : samples) {
        int bin = static_cast<int>((v - binMin) / binWidth);
        bin = std::clamp(bin, 0, HISTOGRAM_BINS - 1);
        histogram[bin]++;
    }
}

inline void TraitDistributions::calculateAll() {
    size.calculate();
    speed.calculate();
    visionRange.calculate();
    efficiency.calculate();
    aggression.calculate();
    reproductionRate.calculate();
    lifespan.calculate();
    mutationRate.calculate();

    // Build histograms with appropriate ranges
    size.buildHistogram(0.0f, 3.0f);
    speed.buildHistogram(0.0f, 10.0f);
    visionRange.buildHistogram(0.0f, 100.0f);
    efficiency.buildHistogram(0.0f, 2.0f);
    aggression.buildHistogram(0.0f, 1.0f);
    reproductionRate.buildHistogram(0.0f, 1.0f);
    lifespan.buildHistogram(0.0f, 200.0f);
    mutationRate.buildHistogram(0.0f, 0.5f);
}

inline void TraitDistributions::calculateCorrelations() {
    // Correlation calculation between all trait pairs
    std::array<TraitStatistics*, 8> traits = {
        &size, &speed, &visionRange, &efficiency,
        &aggression, &reproductionRate, &lifespan, &mutationRate
    };

    for (size_t i = 0; i < 8; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            if (i == j) {
                correlations[i][j] = 1.0f;
                continue;
            }

            const auto& samplesI = traits[i]->samples;
            const auto& samplesJ = traits[j]->samples;

            size_t n = std::min(samplesI.size(), samplesJ.size());
            if (n < 2) {
                correlations[i][j] = 0.0f;
                continue;
            }

            float meanI = traits[i]->mean;
            float meanJ = traits[j]->mean;
            float stdI = traits[i]->stdDev;
            float stdJ = traits[j]->stdDev;

            if (stdI < 0.0001f || stdJ < 0.0001f) {
                correlations[i][j] = 0.0f;
                continue;
            }

            float covariance = 0.0f;
            for (size_t k = 0; k < n; ++k) {
                covariance += (samplesI[k] - meanI) * (samplesJ[k] - meanJ);
            }
            covariance /= static_cast<float>(n);

            correlations[i][j] = covariance / (stdI * stdJ);
        }
    }
}

inline const PopulationSample& StatisticsDataManager::getCurrentPopulation() const {
    return m_currentPopulation;
}

inline const FitnessSample& StatisticsDataManager::getCurrentFitness() const {
    return m_currentFitness;
}

inline const EnergyFlowSample& StatisticsDataManager::getCurrentEnergyFlow() const {
    return m_currentEnergyFlow;
}

inline const SelectionPressureSample& StatisticsDataManager::getCurrentSelectionPressures() const {
    return m_currentSelectionPressures;
}

inline const NicheOccupancySample& StatisticsDataManager::getCurrentNicheOccupancy() const {
    return m_currentNicheOccupancy;
}

} // namespace statistics
} // namespace ui
