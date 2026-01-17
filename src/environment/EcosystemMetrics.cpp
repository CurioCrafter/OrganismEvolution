#include "EcosystemMetrics.h"
#include "ProducerSystem.h"
#include "DecomposerSystem.h"
#include "SeasonManager.h"
#include <cmath>
#include <algorithm>

EcosystemMetrics::EcosystemMetrics()
    : speciesDiversity(1.0f)
    , trophicBalance(10.0f)
    , totalEnergy(0.0f)
    , producerBiomass(0.0f)
    , consumerBiomass(0.0f)
    , decomposerBiomass(0.0f)
    , populationVariance(0.0f)
    , healthScore(100.0f)
{
    // Reserve space for history
    healthHistory.reserve(maxHistorySize);
    herbivoreHistory.reserve(maxHistorySize);
    carnivoreHistory.reserve(maxHistorySize);
}

void EcosystemMetrics::update(float deltaTime,
                              const PopulationCounts& populations,
                              const ProducerSystem* producers,
                              const DecomposerSystem* decomposers,
                              const SeasonManager* seasons) {
    currentPopulations = populations;

    // Update biomass metrics
    if (producers) {
        producerBiomass = producers->getTotalBiomass();
        energyFlow.sunlightToProducers = producerBiomass * 0.01f;  // Approx energy capture rate
    }

    if (decomposers) {
        decomposerBiomass = decomposers->getTotalBiomass();
    }

    // Consumer biomass approximation (would need creature sizes)
    consumerBiomass = populations.getTotal() * 50.0f;  // Rough estimate

    totalEnergy = producerBiomass + consumerBiomass + decomposerBiomass;

    // Calculate metrics
    calculateDiversity();
    calculateTrophicBalance();
    checkWarnings();
    calculateHealthScore();

    // Record history periodically
    timeSinceLastRecord += deltaTime;
    if (timeSinceLastRecord >= recordInterval) {
        recordHistory();
        timeSinceLastRecord = 0.0f;

        // Reset energy flow counters for next period
        energyFlow.producersToHerbivores = 0.0f;
        energyFlow.herbivoresToCarnivores = 0.0f;
        energyFlow.toDecomposers = 0.0f;
        energyFlow.recycledToSoil = 0.0f;
    }
}

void EcosystemMetrics::calculateDiversity() {
    // Shannon diversity index
    // H = -sum(pi * ln(pi)) where pi is proportion of species i

    int total = currentPopulations.getTotal();
    if (total == 0) {
        speciesDiversity = 0.0f;
        return;
    }

    std::vector<int> counts = {
        currentPopulations.grazers,
        currentPopulations.browsers,
        currentPopulations.frugivores,
        currentPopulations.smallPredators,
        currentPopulations.omnivores,
        currentPopulations.apexPredators,
        currentPopulations.scavengers,
        currentPopulations.parasites,
        currentPopulations.cleaners
    };

    float H = 0.0f;
    int speciesPresent = 0;

    for (int count : counts) {
        if (count > 0) {
            speciesPresent++;
            float p = static_cast<float>(count) / total;
            H -= p * std::log(p);
        }
    }

    // Normalize by maximum possible diversity (ln(number of species))
    float maxH = std::log(static_cast<float>(speciesPresent));
    speciesDiversity = (maxH > 0) ? H / maxH : 0.0f;
}

void EcosystemMetrics::calculateTrophicBalance() {
    int herbivores = currentPopulations.getTotalHerbivores();
    int carnivores = currentPopulations.getTotalCarnivores() + currentPopulations.omnivores;

    if (carnivores == 0) {
        trophicBalance = (herbivores > 0) ? 100.0f : 0.0f;  // Infinite ratio or empty
    } else {
        trophicBalance = static_cast<float>(herbivores) / carnivores;
    }
}

void EcosystemMetrics::checkWarnings() {
    warnings.clear();

    int herbivores = currentPopulations.getTotalHerbivores();
    int carnivores = currentPopulations.getTotalCarnivores();

    // Check herbivore population
    if (herbivores < minHerbivoreThreshold / 2) {
        warnings.push_back({
            EcosystemWarning::Severity::CRITICAL,
            EcosystemWarning::Type::LOW_HERBIVORES,
            "Herbivore population critically low!",
            static_cast<float>(herbivores),
            static_cast<float>(minHerbivoreThreshold / 2)
        });
    } else if (herbivores < minHerbivoreThreshold) {
        warnings.push_back({
            EcosystemWarning::Severity::WARNING,
            EcosystemWarning::Type::LOW_HERBIVORES,
            "Herbivore population below threshold",
            static_cast<float>(herbivores),
            static_cast<float>(minHerbivoreThreshold)
        });
    }

    // Check carnivore population
    if (carnivores < minCarnivoreThreshold / 2) {
        warnings.push_back({
            EcosystemWarning::Severity::CRITICAL,
            EcosystemWarning::Type::LOW_CARNIVORES,
            "Carnivore population critically low!",
            static_cast<float>(carnivores),
            static_cast<float>(minCarnivoreThreshold / 2)
        });
    } else if (carnivores < minCarnivoreThreshold) {
        warnings.push_back({
            EcosystemWarning::Severity::WARNING,
            EcosystemWarning::Type::LOW_CARNIVORES,
            "Carnivore population below threshold",
            static_cast<float>(carnivores),
            static_cast<float>(minCarnivoreThreshold)
        });
    }

    // Check trophic balance
    if (trophicBalance < 3.0f) {
        warnings.push_back({
            EcosystemWarning::Severity::WARNING,
            EcosystemWarning::Type::TROPHIC_IMBALANCE,
            "Too many predators relative to prey",
            trophicBalance,
            3.0f
        });
    } else if (trophicBalance > 30.0f) {
        warnings.push_back({
            EcosystemWarning::Severity::WARNING,
            EcosystemWarning::Type::TROPHIC_IMBALANCE,
            "Too few predators, prey overabundant",
            trophicBalance,
            30.0f
        });
    }

    // Check species diversity
    if (speciesDiversity < 0.3f) {
        warnings.push_back({
            EcosystemWarning::Severity::WARNING,
            EcosystemWarning::Type::EXTINCTION_RISK,
            "Low species diversity - ecosystem fragile",
            speciesDiversity,
            0.3f
        });
    }
}

void EcosystemMetrics::calculateHealthScore() {
    healthScore = 100.0f;

    // Penalize trophic imbalance
    float idealRatio = 10.0f;
    float ratioDeviation = std::abs(trophicBalance - idealRatio) / idealRatio;
    healthScore -= ratioDeviation * 20.0f;

    // Penalize low diversity
    healthScore -= (1.0f - speciesDiversity) * 20.0f;

    // Penalize warnings
    for (const auto& warning : warnings) {
        if (warning.severity == EcosystemWarning::Severity::CRITICAL) {
            healthScore -= 15.0f;
        } else if (warning.severity == EcosystemWarning::Severity::WARNING) {
            healthScore -= 5.0f;
        }
    }

    // Penalize very high population variance
    healthScore -= populationVariance * 0.1f;

    // Clamp to valid range
    healthScore = std::clamp(healthScore, 0.0f, 100.0f);
}

void EcosystemMetrics::recordHistory() {
    // Add to history, removing oldest if at capacity
    if (healthHistory.size() >= maxHistorySize) {
        healthHistory.erase(healthHistory.begin());
        herbivoreHistory.erase(herbivoreHistory.begin());
        carnivoreHistory.erase(carnivoreHistory.begin());
    }

    healthHistory.push_back(healthScore);
    herbivoreHistory.push_back(currentPopulations.getTotalHerbivores());
    carnivoreHistory.push_back(currentPopulations.getTotalCarnivores());

    // Calculate population variance from recent history
    if (herbivoreHistory.size() >= 10) {
        float sum = 0.0f;
        float sumSq = 0.0f;
        size_t historySize = herbivoreHistory.size();
        size_t n = std::min<size_t>(50, historySize);
        size_t start = historySize - n;

        for (size_t i = start; i < historySize; i++) {
            sum += herbivoreHistory[i];
            sumSq += herbivoreHistory[i] * herbivoreHistory[i];
        }

        float mean = sum / static_cast<float>(n);
        populationVariance = (sumSq / static_cast<float>(n)) - (mean * mean);
    }
}

bool EcosystemMetrics::hasCriticalWarnings() const {
    for (const auto& warning : warnings) {
        if (warning.severity == EcosystemWarning::Severity::CRITICAL) {
            return true;
        }
    }
    return false;
}

void EcosystemMetrics::setWarningThresholds(int minHerbivores, int minCarnivores, float minProducerCov) {
    minHerbivoreThreshold = minHerbivores;
    minCarnivoreThreshold = minCarnivores;
    minProducerCoverage = minProducerCov;
}
