#pragma once

#include <vector>
#include <map>
#include <string>
#include "../entities/CreatureType.h"

class ProducerSystem;
class DecomposerSystem;
class SeasonManager;

// Tracks population counts by creature type
struct PopulationCounts {
    int grazers = 0;
    int browsers = 0;
    int frugivores = 0;
    int smallPredators = 0;
    int omnivores = 0;
    int apexPredators = 0;
    int scavengers = 0;
    int parasites = 0;
    int cleaners = 0;

    int getTotalHerbivores() const { return grazers + browsers + frugivores; }
    int getTotalCarnivores() const { return smallPredators + apexPredators; }
    int getTotal() const {
        return grazers + browsers + frugivores + smallPredators +
               omnivores + apexPredators + scavengers + parasites + cleaners;
    }
};

// Energy flow tracking
struct EnergyFlowMetrics {
    float sunlightToProducers;    // Energy captured by plants
    float producersToHerbivores;  // Energy consumed by herbivores
    float herbivoresToCarnivores; // Energy consumed by carnivores
    float toDecomposers;          // Energy from corpses
    float recycledToSoil;         // Nutrients returned to soil

    float getTotalSystemEnergy() const {
        return sunlightToProducers;  // Primary energy input
    }

    float getTransferEfficiency(int fromLevel, int toLevel) const {
        if (fromLevel == 1 && toLevel == 2) {
            return sunlightToProducers > 0 ?
                   producersToHerbivores / sunlightToProducers : 0;
        }
        if (fromLevel == 2 && toLevel == 3) {
            return producersToHerbivores > 0 ?
                   herbivoresToCarnivores / producersToHerbivores : 0;
        }
        return 0;
    }
};

// Health warnings and thresholds
struct EcosystemWarning {
    enum class Severity { INFO, WARNING, CRITICAL };
    enum class Type {
        LOW_HERBIVORES,
        LOW_CARNIVORES,
        LOW_PRODUCERS,
        TROPHIC_IMBALANCE,
        EXTINCTION_RISK,
        OVERPOPULATION,
        NUTRIENT_DEPLETION
    };

    Severity severity;
    Type type;
    std::string message;
    float value;
    float threshold;
};

class EcosystemMetrics {
public:
    EcosystemMetrics();

    void update(float deltaTime,
                const PopulationCounts& populations,
                const ProducerSystem* producers,
                const DecomposerSystem* decomposers,
                const SeasonManager* seasons);

    // Population metrics
    const PopulationCounts& getPopulations() const { return currentPopulations; }
    float getSpeciesDiversity() const { return speciesDiversity; }
    float getTrophicBalance() const { return trophicBalance; }

    // Energy metrics
    const EnergyFlowMetrics& getEnergyFlow() const { return energyFlow; }
    float getTotalEnergyInSystem() const { return totalEnergy; }

    // Biomass metrics
    float getProducerBiomass() const { return producerBiomass; }
    float getConsumerBiomass() const { return consumerBiomass; }
    float getDecomposerBiomass() const { return decomposerBiomass; }

    // Stability metrics
    float getPopulationVariance() const { return populationVariance; }
    float getEcosystemHealthScore() const { return healthScore; }

    // Warnings
    const std::vector<EcosystemWarning>& getWarnings() const { return warnings; }
    bool hasCriticalWarnings() const;

    // Historical data for graphs
    const std::vector<float>& getHealthHistory() const { return healthHistory; }
    const std::vector<int>& getHerbivoreHistory() const { return herbivoreHistory; }
    const std::vector<int>& getCarnivoreHistory() const { return carnivoreHistory; }

    // Track energy flow events (called by creatures/systems)
    void recordEnergyToHerbivore(float amount) { energyFlow.producersToHerbivores += amount; }
    void recordEnergyToCarnivore(float amount) { energyFlow.herbivoresToCarnivores += amount; }
    void recordEnergyToDecomposer(float amount) { energyFlow.toDecomposers += amount; }

    // Configuration
    void setWarningThresholds(int minHerbivores, int minCarnivores, float minProducerCoverage);

private:
    PopulationCounts currentPopulations;
    EnergyFlowMetrics energyFlow;

    float speciesDiversity;     // Shannon diversity index (0-1)
    float trophicBalance;       // Ratio health (ideal is ~10:1)
    float totalEnergy;
    float producerBiomass;
    float consumerBiomass;
    float decomposerBiomass;

    float populationVariance;   // Rolling variance over time
    float healthScore;          // Overall health (0-100)

    std::vector<EcosystemWarning> warnings;

    // Historical tracking
    std::vector<float> healthHistory;
    std::vector<int> herbivoreHistory;
    std::vector<int> carnivoreHistory;
    static constexpr int maxHistorySize = 500;

    // Warning thresholds
    int minHerbivoreThreshold = 20;
    int minCarnivoreThreshold = 5;
    float minProducerCoverage = 0.4f;

    // Timing
    float timeSinceLastRecord = 0.0f;
    static constexpr float recordInterval = 1.0f;  // Record every second

    void calculateDiversity();
    void calculateTrophicBalance();
    void calculateHealthScore();
    void checkWarnings();
    void recordHistory();
};
