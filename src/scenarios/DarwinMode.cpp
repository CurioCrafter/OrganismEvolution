#include "DarwinMode.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

DarwinMode::DarwinMode() {
    m_divergenceData.reserve(64);  // Max 8 islands * 7 pairs / 2
    m_adaptiveRadiations.reserve(16);
    m_founderEffects.reserve(16);
}

// ============================================================================
// Initialization
// ============================================================================

void DarwinMode::init(MultiIslandManager& islands) {
    m_islands = &islands;

    // Initialize divergence data for all island pairs
    uint32_t islandCount = islands.getIslandCount();

    for (uint32_t i = 0; i < islandCount; ++i) {
        for (uint32_t j = i + 1; j < islandCount; ++j) {
            DivergenceData data;
            data.islandA = i;
            data.islandB = j;
            data.geneticDistance = 0.0f;
            data.timeSinceSplit = 0.0f;
            m_divergenceData.push_back(data);
        }
    }

    // Initialize founder effects for each island
    for (uint32_t i = 0; i < islandCount; ++i) {
        const auto* island = islands.getIsland(i);
        if (!island) continue;

        FounderEffectRecord record;
        record.islandIndex = i;
        record.islandName = island->name;
        record.colonizationTime = 0.0f;
        record.founderCount = island->stats.totalCreatures;
        record.initialGeneticDiversity = island->stats.geneticDiversity;
        record.currentGeneticDiversity = island->stats.geneticDiversity;
        record.bottleneckSeverity = 0.0f;
        record.recoveryProgress = 1.0f;

        m_founderEffects.push_back(record);
    }

    m_totalTime = 0.0f;
    m_currentGeneration = 0;
}

void DarwinMode::reset() {
    m_divergenceData.clear();
    m_adaptiveRadiations.clear();
    m_founderEffects.clear();
    m_divergenceHistory.clear();
    m_totalTime = 0.0f;
    m_timeSinceLastUpdate = 0.0f;
    m_currentGeneration = 0;

    if (m_islands) {
        init(*m_islands);
    }
}

// ============================================================================
// Update
// ============================================================================

void DarwinMode::update(float deltaTime) {
    if (!m_enabled || !m_islands) return;

    m_totalTime += deltaTime;
    m_timeSinceLastUpdate += deltaTime;

    // Update at configured interval
    if (m_timeSinceLastUpdate >= m_config.divergenceUpdateInterval) {
        m_timeSinceLastUpdate = 0.0f;
        m_currentGeneration++;

        updateDivergenceData();
        updateAdaptiveRadiations();
        updateFounderEffects();
        checkForSpeciation();
        checkForNewRadiation();
    }
}

void DarwinMode::updateDivergenceData() {
    for (auto& data : m_divergenceData) {
        // Calculate current metrics
        float oldDistance = data.geneticDistance;

        data.geneticDistance = calculateGeneticDistance(data.islandA, data.islandB);
        data.morphologicalDivergence = calculateMorphologicalDivergence(data.islandA, data.islandB);
        data.behavioralDivergence = calculateBehavioralDivergence(data.islandA, data.islandB);
        data.reproductiveIsolation = calculateReproductiveIsolation(data.islandA, data.islandB);

        // Update time tracking
        data.timeSinceSplit = m_totalTime;
        data.generationsSinceSplit = m_currentGeneration;

        // Calculate trend
        recordDivergenceHistory(data.islandA, data.islandB, data.geneticDistance);
        data.geneticDistanceTrend = calculateDivergenceTrend(data.islandA, data.islandB);

        // Calculate divergence rate
        if (oldDistance > 0.001f) {
            data.divergenceRate = (data.geneticDistance - oldDistance) / m_config.divergenceUpdateInterval;
        }

        // Check for speciation
        if (!data.speciationComplete &&
            data.geneticDistance >= m_config.speciationGeneticThreshold &&
            data.reproductiveIsolation >= m_config.speciationIsolationThreshold) {
            data.speciationComplete = true;
        }
    }
}

void DarwinMode::updateAdaptiveRadiations() {
    for (auto& radiation : m_adaptiveRadiations) {
        if (!radiation.isActive) continue;

        radiation.currentGeneration = m_currentGeneration;

        // Calculate radiation rate
        int speciesCount = static_cast<int>(radiation.descendantSpeciesIds.size());
        int generationsDiff = radiation.currentGeneration - radiation.startGeneration;

        if (generationsDiff > 0) {
            radiation.radiationRate = static_cast<float>(speciesCount) / generationsDiff;
        }

        // Check if radiation has slowed
        if (radiation.radiationRate < m_config.radiationRateThreshold * 0.1f &&
            generationsDiff > 10) {
            radiation.isActive = false;
        }
    }
}

void DarwinMode::updateFounderEffects() {
    if (!m_islands) return;

    for (auto& record : m_founderEffects) {
        const auto* island = m_islands->getIsland(record.islandIndex);
        if (!island) continue;

        record.currentGeneticDiversity = island->stats.geneticDiversity;

        // Calculate bottleneck severity
        if (record.initialGeneticDiversity > 0.01f) {
            float diversityLoss = 1.0f - (record.currentGeneticDiversity / record.initialGeneticDiversity);
            record.bottleneckSeverity = std::max(0.0f, diversityLoss);
        }

        // Calculate recovery progress (diversity compared to theoretical maximum)
        float theoreticalMax = 0.8f;  // Simplified max diversity
        record.recoveryProgress = std::min(1.0f, record.currentGeneticDiversity / theoreticalMax);

        // Calculate divergence from ancestor (using first island as reference)
        if (record.islandIndex > 0) {
            record.divergenceFromAncestor = calculateGeneticDistance(0, record.islandIndex);
        }

        // Identify unique traits
        record.uniqueTraits = identifyUniqueTraits(record.islandIndex);
    }
}

void DarwinMode::checkForSpeciation() {
    // Already handled in updateDivergenceData via speciationComplete flag
    // This method can trigger additional events or notifications
}

void DarwinMode::checkForNewRadiation() {
    if (!m_islands) return;

    // Check if any island is experiencing rapid species formation
    for (uint32_t i = 0; i < m_islands->getIslandCount(); ++i) {
        const auto* island = m_islands->getIsland(i);
        if (!island) continue;

        // Check if this island already has an active radiation
        bool hasActiveRadiation = false;
        for (const auto& radiation : m_adaptiveRadiations) {
            if (radiation.originIsland == i && radiation.isActive) {
                hasActiveRadiation = true;
                break;
            }
        }

        if (hasActiveRadiation) continue;

        // Check criteria for new radiation event
        int speciesCount = island->stats.speciesCount;

        if (speciesCount >= m_config.radiationNicheThreshold) {
            // New adaptive radiation detected
            AdaptiveRadiationEvent radiation;
            radiation.originIsland = i;
            radiation.startTime = m_totalTime;
            radiation.startGeneration = m_currentGeneration;
            radiation.currentGeneration = m_currentGeneration;
            radiation.nichesFilled = speciesCount;
            radiation.isActive = true;

            // In a full implementation, this would track actual species IDs
            for (int s = 0; s < speciesCount; ++s) {
                radiation.descendantSpeciesIds.push_back(s);  // Placeholder
            }

            radiation.niches = identifyNiches(radiation.descendantSpeciesIds);

            m_adaptiveRadiations.push_back(radiation);
        }
    }
}

// ============================================================================
// Divergence Access
// ============================================================================

const DivergenceData* DarwinMode::getDivergence(uint32_t islandA, uint32_t islandB) const {
    // Ensure consistent ordering
    if (islandA > islandB) std::swap(islandA, islandB);

    for (const auto& data : m_divergenceData) {
        if (data.islandA == islandA && data.islandB == islandB) {
            return &data;
        }
    }
    return nullptr;
}

std::vector<const DivergenceData*> DarwinMode::getMostDivergent(int count) const {
    std::vector<const DivergenceData*> sorted;
    for (const auto& data : m_divergenceData) {
        sorted.push_back(&data);
    }

    std::sort(sorted.begin(), sorted.end(),
              [](const DivergenceData* a, const DivergenceData* b) {
                  return a->geneticDistance > b->geneticDistance;
              });

    if (static_cast<int>(sorted.size()) > count) {
        sorted.resize(count);
    }

    return sorted;
}

std::vector<const DivergenceData*> DarwinMode::getNearSpeciation(float threshold) const {
    std::vector<const DivergenceData*> result;

    for (const auto& data : m_divergenceData) {
        if (!data.speciationComplete && data.geneticDistance >= threshold) {
            result.push_back(&data);
        }
    }

    // Sort by how close to speciation
    std::sort(result.begin(), result.end(),
              [](const DivergenceData* a, const DivergenceData* b) {
                  return a->geneticDistance > b->geneticDistance;
              });

    return result;
}

// ============================================================================
// Adaptive Radiation
// ============================================================================

bool DarwinMode::isAdaptiveRadiationOccurring() const {
    for (const auto& radiation : m_adaptiveRadiations) {
        if (radiation.isActive) return true;
    }
    return false;
}

int DarwinMode::getActiveRadiationCount() const {
    int count = 0;
    for (const auto& radiation : m_adaptiveRadiations) {
        if (radiation.isActive) count++;
    }
    return count;
}

// ============================================================================
// Founder Effects
// ============================================================================

std::vector<const FounderEffectRecord*> DarwinMode::getStrongFounderEffects(float threshold) const {
    std::vector<const FounderEffectRecord*> result;

    for (const auto& record : m_founderEffects) {
        if (record.bottleneckSeverity >= threshold) {
            result.push_back(&record);
        }
    }

    // Sort by severity
    std::sort(result.begin(), result.end(),
              [](const FounderEffectRecord* a, const FounderEffectRecord* b) {
                  return a->bottleneckSeverity > b->bottleneckSeverity;
              });

    return result;
}

// ============================================================================
// Summary
// ============================================================================

DarwinMode::DarwinSummary DarwinMode::getSummary() const {
    DarwinSummary summary;

    if (!m_islands) return summary;

    summary.totalIslands = m_islands->getIslandCount();

    // Calculate average and max genetic distance
    float totalDistance = 0.0f;
    float maxDistance = 0.0f;

    for (const auto& data : m_divergenceData) {
        totalDistance += data.geneticDistance;
        maxDistance = std::max(maxDistance, data.geneticDistance);

        if (data.speciationComplete) {
            summary.completedSpeciations++;
        } else if (data.geneticDistance > m_config.speciationGeneticThreshold * 0.7f) {
            summary.activeSpeciationEvents++;
        }
    }

    if (!m_divergenceData.empty()) {
        summary.averageGeneticDistance = totalDistance / m_divergenceData.size();
    }
    summary.maxGeneticDistance = maxDistance;

    // Count islands with unique species
    for (const auto& record : m_founderEffects) {
        if (record.divergenceFromAncestor > 0.3f || !record.uniqueTraits.empty()) {
            summary.islandsWithUniqueSpecies++;
        }
    }

    // Adaptive radiations
    summary.adaptiveRadiations = getActiveRadiationCount();

    // Founder events
    for (const auto& record : m_founderEffects) {
        if (record.founderCount < m_config.founderBottleneckSize) {
            summary.founderEvents++;
        }
    }

    // Overall divergence rate
    float totalRate = 0.0f;
    for (const auto& data : m_divergenceData) {
        totalRate += data.divergenceRate;
    }
    if (!m_divergenceData.empty()) {
        summary.overallDivergenceRate = totalRate / m_divergenceData.size();
    }

    return summary;
}

// ============================================================================
// Calculation Helpers
// ============================================================================

float DarwinMode::calculateGeneticDistance(uint32_t islandA, uint32_t islandB) const {
    if (!m_islands) return 0.0f;

    // Use the island manager's built-in genetic distance
    return m_islands->getGeneticDistance(islandA, islandB);
}

float DarwinMode::calculateMorphologicalDivergence(uint32_t islandA, uint32_t islandB) const {
    if (!m_islands) return 0.0f;

    const auto* a = m_islands->getIsland(islandA);
    const auto* b = m_islands->getIsland(islandB);

    if (!a || !b) return 0.0f;

    // Simplified: compare average fitness as proxy for morphology
    // In full implementation, compare actual phenotypic traits
    float fitnessDiff = std::abs(a->stats.avgFitness - b->stats.avgFitness);

    // Normalize to 0-1
    return std::min(1.0f, fitnessDiff / 0.5f);
}

float DarwinMode::calculateBehavioralDivergence(uint32_t islandA, uint32_t islandB) const {
    if (!m_islands) return 0.0f;

    // Simplified: use species composition difference as proxy
    const auto* a = m_islands->getIsland(islandA);
    const auto* b = m_islands->getIsland(islandB);

    if (!a || !b) return 0.0f;

    // Compare species counts (simplified)
    float speciesDiff = std::abs(a->stats.speciesCount - b->stats.speciesCount);

    return std::min(1.0f, speciesDiff / 10.0f);
}

float DarwinMode::calculateReproductiveIsolation(uint32_t islandA, uint32_t islandB) const {
    if (!m_islands) return 0.0f;

    // Reproductive isolation increases with:
    // 1. Physical distance
    // 2. Genetic distance
    // 3. Time since separation

    float physicalDist = m_islands->getIslandDistance(islandA, islandB);
    float geneticDist = calculateGeneticDistance(islandA, islandB);

    // Normalize physical distance (assume 500 units = max)
    float physicalFactor = std::min(1.0f, physicalDist / 500.0f);

    // Combine factors
    float isolation = physicalFactor * 0.3f + geneticDist * 0.7f;

    return std::min(1.0f, isolation);
}

// ============================================================================
// Trait Analysis
// ============================================================================

std::vector<std::string> DarwinMode::identifyUniqueTraits(uint32_t islandIndex) const {
    std::vector<std::string> traits;

    if (!m_islands) return traits;

    const auto* island = m_islands->getIsland(islandIndex);
    if (!island) return traits;

    // Compare with global averages
    auto globalStats = m_islands->getGlobalStats();

    // Check for unique fitness
    if (island->stats.avgFitness > globalStats.avgFitness * 1.2f) {
        traits.push_back("High fitness");
    } else if (island->stats.avgFitness < globalStats.avgFitness * 0.8f) {
        traits.push_back("Low fitness");
    }

    // Check for unique diversity
    if (island->stats.geneticDiversity > globalStats.geneticDiversity * 1.3f) {
        traits.push_back("High diversity");
    } else if (island->stats.geneticDiversity < globalStats.geneticDiversity * 0.7f) {
        traits.push_back("Genetic bottleneck");
    }

    // Check for unique species count
    float avgSpecies = static_cast<float>(globalStats.speciesCount) / m_islands->getIslandCount();
    if (island->stats.speciesCount > avgSpecies * 1.5f) {
        traits.push_back("High speciation");
    }

    return traits;
}

std::vector<std::string> DarwinMode::identifyNiches(const std::vector<uint32_t>& speciesIds) const {
    std::vector<std::string> niches;

    // Simplified niche identification
    // In full implementation, analyze actual ecological data

    if (speciesIds.size() >= 2) {
        niches.push_back("Primary consumer");
    }
    if (speciesIds.size() >= 3) {
        niches.push_back("Secondary consumer");
    }
    if (speciesIds.size() >= 4) {
        niches.push_back("Apex predator");
    }
    if (speciesIds.size() >= 5) {
        niches.push_back("Decomposer");
    }
    if (speciesIds.size() >= 6) {
        niches.push_back("Specialist");
    }

    return niches;
}

// ============================================================================
// History Management
// ============================================================================

void DarwinMode::recordDivergenceHistory(uint32_t islandA, uint32_t islandB, float distance) {
    auto key = std::make_pair(std::min(islandA, islandB), std::max(islandA, islandB));

    auto& history = m_divergenceHistory[key];
    history.push_back(distance);

    // Trim old history
    while (static_cast<int>(history.size()) > m_config.historyLength) {
        history.erase(history.begin());
    }
}

float DarwinMode::calculateDivergenceTrend(uint32_t islandA, uint32_t islandB) const {
    auto key = std::make_pair(std::min(islandA, islandB), std::max(islandA, islandB));

    auto it = m_divergenceHistory.find(key);
    if (it == m_divergenceHistory.end() || it->second.size() < 2) {
        return 0.0f;
    }

    const auto& history = it->second;

    // Simple linear regression for trend
    size_t n = history.size();
    float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumXX = 0.0f;

    for (size_t i = 0; i < n; ++i) {
        float x = static_cast<float>(i);
        float y = history[i];
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }

    float nf = static_cast<float>(n);
    float denominator = nf * sumXX - sumX * sumX;

    if (std::abs(denominator) < 0.001f) {
        return 0.0f;
    }

    float slope = (nf * sumXY - sumX * sumY) / denominator;

    return slope;  // Positive = diverging, negative = converging
}

} // namespace Forge
