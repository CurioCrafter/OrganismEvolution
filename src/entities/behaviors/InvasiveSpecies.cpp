#include "InvasiveSpecies.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

InvasiveSpecies::InvasiveSpecies() {
    m_invasionHistory.reserve(100);
    m_recentAlerts.reserve(MAX_ALERTS);
}

// ============================================================================
// Invasion Tracking
// ============================================================================

void InvasiveSpecies::trackInvasion(uint32_t creatureId, uint32_t originIsland, uint32_t targetIsland) {
    // Simple tracking with minimal data
    trackInvasion(0, "Unknown Species", originIsland, targetIsland, 1, m_totalTime);
}

void InvasiveSpecies::trackInvasion(uint32_t speciesId, const std::string& speciesName,
                                     uint32_t originIsland, uint32_t targetIsland,
                                     int founderCount, float timestamp) {
    // Check if this species is already being tracked as invasive on this island
    for (auto& record : m_invasionHistory) {
        if (record.speciesId == speciesId &&
            record.targetIsland == targetIsland &&
            record.currentPhase != InvasionPhase::EXTINCTION &&
            record.currentPhase != InvasionPhase::DECLINE) {
            // Add to existing invasion
            record.currentPopulation += founderCount;
            return;
        }
    }

    // Create new invasion record
    InvasionRecord record;
    record.id = m_nextInvasionId++;
    record.speciesId = speciesId;
    record.speciesName = speciesName;
    record.originIsland = originIsland;
    record.targetIsland = targetIsland;
    record.arrivalTime = timestamp;
    record.currentTime = timestamp;
    record.founderCount = founderCount;
    record.currentPopulation = founderCount;
    record.currentPhase = InvasionPhase::ARRIVAL;

    // Initial phase history
    record.phaseHistory.push_back({timestamp, InvasionPhase::ARRIVAL});

    m_invasionHistory.push_back(record);

    // Update stats
    m_stats.totalInvasions++;
    m_stats.ongoingInvasions++;
    m_stats.invasionsByTargetIsland[targetIsland]++;
    m_stats.invasionsByOriginIsland[originIsland]++;

    // Emit alert
    std::stringstream msg;
    msg << "New arrival: " << speciesName << " on island " << targetIsland
        << " (from island " << originIsland << ", " << founderCount << " individuals)";
    emitAlert(InvasionAlert::Severity::INFO, record.id, msg.str());
}

// ============================================================================
// Update
// ============================================================================

void InvasiveSpecies::update(float deltaTime, MultiIslandManager& islands) {
    m_totalTime += deltaTime;

    for (auto& record : m_invasionHistory) {
        if (record.currentPhase == InvasionPhase::EXTINCTION) {
            continue;  // Skip extinct invasions
        }

        record.currentTime = m_totalTime;

        // Update population count
        int oldPopulation = record.currentPopulation;
        record.currentPopulation = countInvasivePopulation(record, islands);

        // Track peak population
        if (record.currentPopulation > record.peakPopulation) {
            record.peakPopulation = record.currentPopulation;
            record.peakTime = m_totalTime;
        }

        // Update phase
        updateInvasionPhase(record, islands);

        // Assess ecological impact
        assessEcologicalImpact(record, islands);

        // Check for species displacement
        checkForDisplacement(record, islands);
    }

    updateStatistics();
}

void InvasiveSpecies::updateInvasionPhase(InvasionRecord& record, MultiIslandManager& islands) {
    InvasionPhase oldPhase = record.currentPhase;
    InvasionPhase newPhase = oldPhase;

    int population = record.currentPopulation;
    const auto* targetIsland = islands.getIsland(record.targetIsland);

    if (!targetIsland) {
        newPhase = InvasionPhase::EXTINCTION;
    }
    else if (population == 0) {
        newPhase = InvasionPhase::EXTINCTION;
    }
    else if (population < 5 && record.currentPhase != InvasionPhase::ARRIVAL) {
        newPhase = InvasionPhase::DECLINE;
    }
    else {
        int totalIslandPop = targetIsland->stats.totalCreatures;
        float populationRatio = totalIslandPop > 0 ?
            static_cast<float>(population) / totalIslandPop : 0.0f;

        switch (record.currentPhase) {
            case InvasionPhase::ARRIVAL:
                if (population >= m_establishmentThreshold) {
                    newPhase = InvasionPhase::ESTABLISHMENT;
                    record.established = true;
                    record.establishmentTime = m_totalTime;

                    std::stringstream msg;
                    msg << record.speciesName << " has established on island "
                        << record.targetIsland << " (" << population << " individuals)";
                    emitAlert(InvasionAlert::Severity::WARNING, record.id, msg.str());
                }
                break;

            case InvasionPhase::ESTABLISHMENT:
                if (population > record.peakPopulation * 0.5f &&
                    population > m_establishmentThreshold * 2) {
                    newPhase = InvasionPhase::EXPANSION;
                }
                break;

            case InvasionPhase::EXPANSION:
                if (populationRatio > m_dominanceThreshold) {
                    newPhase = InvasionPhase::DOMINANT;

                    std::stringstream msg;
                    msg << "CRITICAL: " << record.speciesName << " has become dominant on island "
                        << record.targetIsland << " (" << static_cast<int>(populationRatio * 100)
                        << "% of population)";
                    emitAlert(InvasionAlert::Severity::CRITICAL, record.id, msg.str());
                }
                else if (population < record.peakPopulation * 0.7f) {
                    newPhase = InvasionPhase::INTEGRATION;
                }
                break;

            case InvasionPhase::INTEGRATION:
                if (population < m_establishmentThreshold / 2) {
                    newPhase = InvasionPhase::DECLINE;
                }
                else if (populationRatio > m_dominanceThreshold) {
                    newPhase = InvasionPhase::DOMINANT;
                }
                break;

            case InvasionPhase::DECLINE:
                if (population == 0) {
                    newPhase = InvasionPhase::EXTINCTION;

                    std::stringstream msg;
                    msg << record.speciesName << " invasion on island " << record.targetIsland
                        << " has gone extinct";
                    emitAlert(InvasionAlert::Severity::INFO, record.id, msg.str());
                }
                else if (population >= m_establishmentThreshold) {
                    newPhase = InvasionPhase::ESTABLISHMENT;  // Recovery
                }
                break;

            case InvasionPhase::DOMINANT:
                if (populationRatio < m_dominanceThreshold * 0.5f) {
                    newPhase = InvasionPhase::INTEGRATION;  // No longer dominant
                }
                break;

            default:
                break;
        }
    }

    // Record phase change
    if (newPhase != oldPhase) {
        record.currentPhase = newPhase;
        record.phaseHistory.push_back({m_totalTime, newPhase});

        // Update ongoing count
        if (newPhase == InvasionPhase::EXTINCTION) {
            m_stats.ongoingInvasions--;
            m_stats.failedInvasions++;
        }
        else if (oldPhase == InvasionPhase::ARRIVAL && record.established) {
            m_stats.successfulEstablishments++;
        }
        else if (newPhase == InvasionPhase::DOMINANT) {
            m_stats.dominantInvaders++;
        }
    }
}

void InvasiveSpecies::assessEcologicalImpact(InvasionRecord& record, MultiIslandManager& islands) {
    if (record.currentPopulation == 0) {
        record.impactSeverity = 0.0f;
        return;
    }

    const auto* targetIsland = islands.getIsland(record.targetIsland);
    if (!targetIsland) return;

    // Calculate various impact types
    float competitionImpact = calculateCompetitionImpact(record, islands);
    float predationImpact = calculatePredationImpact(record, islands);

    // Population ratio impact
    int totalPop = targetIsland->stats.totalCreatures;
    float populationRatio = totalPop > 0 ?
        static_cast<float>(record.currentPopulation) / totalPop : 0.0f;

    // Determine primary impact type
    if (record.nativeSpeciesDisplaced > 0) {
        record.primaryImpact = EcologicalImpact::DISPLACEMENT;
    }
    else if (predationImpact > competitionImpact && predationImpact > 0.3f) {
        record.primaryImpact = EcologicalImpact::PREDATION;
    }
    else if (competitionImpact > 0.3f) {
        record.primaryImpact = EcologicalImpact::COMPETITION;
    }
    else if (populationRatio > m_dominanceThreshold) {
        record.primaryImpact = EcologicalImpact::ECOSYSTEM_CHANGE;
    }
    else {
        record.primaryImpact = EcologicalImpact::MINIMAL;
    }

    // Calculate overall severity
    record.impactSeverity = std::max({
        competitionImpact,
        predationImpact,
        populationRatio,
        static_cast<float>(record.nativeSpeciesDisplaced) * 0.2f
    }) * m_impactSensitivity;

    record.impactSeverity = std::min(1.0f, record.impactSeverity);
}

void InvasiveSpecies::checkForDisplacement(InvasionRecord& record, MultiIslandManager& islands) {
    // Simplified displacement check
    // In a full implementation, this would track specific native species populations

    if (record.currentPhase == InvasionPhase::DOMINANT) {
        // Dominant invasives likely displacing natives
        const auto* island = islands.getIsland(record.targetIsland);
        if (island) {
            // Count species that might be affected
            int potentialDisplacements = std::max(0, island->stats.speciesCount - 1);
            record.nativeSpeciesDisplaced = std::max(record.nativeSpeciesDisplaced,
                                                      potentialDisplacements / 3);
        }
    }

    // Update total displacement stat
    int totalDisplacements = 0;
    for (const auto& r : m_invasionHistory) {
        totalDisplacements += r.nativeSpeciesDisplaced;
    }
    m_stats.totalNativeDisplacements = totalDisplacements;
}

// ============================================================================
// Record Access
// ============================================================================

const InvasionRecord* InvasiveSpecies::getInvasion(uint32_t id) const {
    for (const auto& record : m_invasionHistory) {
        if (record.id == id) {
            return &record;
        }
    }
    return nullptr;
}

std::vector<const InvasionRecord*> InvasiveSpecies::getActiveInvasions() const {
    std::vector<const InvasionRecord*> result;
    for (const auto& record : m_invasionHistory) {
        if (record.currentPhase != InvasionPhase::EXTINCTION) {
            result.push_back(&record);
        }
    }
    return result;
}

std::vector<const InvasionRecord*> InvasiveSpecies::getEstablishedInvasions() const {
    std::vector<const InvasionRecord*> result;
    for (const auto& record : m_invasionHistory) {
        if (record.established && record.currentPhase != InvasionPhase::EXTINCTION) {
            result.push_back(&record);
        }
    }
    return result;
}

std::vector<const InvasionRecord*> InvasiveSpecies::getFailedInvasions() const {
    std::vector<const InvasionRecord*> result;
    for (const auto& record : m_invasionHistory) {
        if (record.currentPhase == InvasionPhase::EXTINCTION) {
            result.push_back(&record);
        }
    }
    return result;
}

std::vector<const InvasionRecord*> InvasiveSpecies::getInvasionsOnIsland(uint32_t islandIndex) const {
    std::vector<const InvasionRecord*> result;
    for (const auto& record : m_invasionHistory) {
        if (record.targetIsland == islandIndex) {
            result.push_back(&record);
        }
    }
    return result;
}

std::vector<const InvasionRecord*> InvasiveSpecies::getInvasionsFromIsland(uint32_t islandIndex) const {
    std::vector<const InvasionRecord*> result;
    for (const auto& record : m_invasionHistory) {
        if (record.originIsland == islandIndex) {
            result.push_back(&record);
        }
    }
    return result;
}

// ============================================================================
// Impact Analysis
// ============================================================================

float InvasiveSpecies::calculateIslandInvasiveLoad(uint32_t islandIndex) const {
    float totalLoad = 0.0f;
    int count = 0;

    for (const auto& record : m_invasionHistory) {
        if (record.targetIsland == islandIndex &&
            record.currentPhase != InvasionPhase::EXTINCTION) {
            totalLoad += record.impactSeverity;
            count++;
        }
    }

    return count > 0 ? totalLoad / count : 0.0f;
}

std::vector<const InvasionRecord*> InvasiveSpecies::getMostImpactfulInvasions(int count) const {
    std::vector<const InvasionRecord*> all;
    for (const auto& record : m_invasionHistory) {
        if (record.currentPhase != InvasionPhase::EXTINCTION) {
            all.push_back(&record);
        }
    }

    // Sort by impact severity
    std::sort(all.begin(), all.end(), [](const InvasionRecord* a, const InvasionRecord* b) {
        return a->impactSeverity > b->impactSeverity;
    });

    // Return top N
    if (static_cast<int>(all.size()) > count) {
        all.resize(count);
    }

    return all;
}

bool InvasiveSpecies::isSpeciesInvasive(uint32_t speciesId, uint32_t islandIndex) const {
    for (const auto& record : m_invasionHistory) {
        if (record.speciesId == speciesId &&
            record.targetIsland == islandIndex &&
            record.currentPhase != InvasionPhase::EXTINCTION) {
            return true;
        }
    }
    return false;
}

std::map<EcologicalImpact, int> InvasiveSpecies::getImpactSummary(uint32_t islandIndex) const {
    std::map<EcologicalImpact, int> summary;

    for (const auto& record : m_invasionHistory) {
        if (record.targetIsland == islandIndex &&
            record.currentPhase != InvasionPhase::EXTINCTION) {
            summary[record.primaryImpact]++;
        }
    }

    return summary;
}

// ============================================================================
// Statistics
// ============================================================================

void InvasiveSpecies::resetStats() {
    m_stats = InvasionStats();
}

float InvasiveSpecies::getEstablishmentSuccessRate() const {
    if (m_stats.totalInvasions == 0) return 0.0f;
    return static_cast<float>(m_stats.successfulEstablishments) / m_stats.totalInvasions;
}

float InvasiveSpecies::getAverageTimeToEstablishment() const {
    float total = 0.0f;
    int count = 0;

    for (const auto& record : m_invasionHistory) {
        if (record.established && record.establishmentTime > record.arrivalTime) {
            total += record.establishmentTime - record.arrivalTime;
            count++;
        }
    }

    return count > 0 ? total / count : 0.0f;
}

float InvasiveSpecies::getAverageFounderPopulation() const {
    if (m_invasionHistory.empty()) return 0.0f;

    float total = 0.0f;
    for (const auto& record : m_invasionHistory) {
        total += record.founderCount;
    }

    return total / m_invasionHistory.size();
}

void InvasiveSpecies::updateStatistics() {
    // Update average impact severity
    float totalImpact = 0.0f;
    int impactCount = 0;

    for (const auto& record : m_invasionHistory) {
        if (record.currentPhase != InvasionPhase::EXTINCTION) {
            totalImpact += record.impactSeverity;
            impactCount++;
        }
    }

    m_stats.averageImpactSeverity = impactCount > 0 ? totalImpact / impactCount : 0.0f;
    m_stats.averageEstablishmentRate = getEstablishmentSuccessRate();
}

// ============================================================================
// Population Counting
// ============================================================================

int InvasiveSpecies::countInvasivePopulation(const InvasionRecord& record,
                                              const MultiIslandManager& islands) const {
    const auto* island = islands.getIsland(record.targetIsland);
    if (!island || !island->creatures) return 0;

    // Simplified: count all creatures matching the species ID
    // In a full implementation, this would use proper species tracking
    int count = 0;

    island->creatures->forEach([&count, &record](Creature& creature, size_t idx) {
        if (creature.isAlive()) {
            // Match by species ID or genetic similarity
            // Simplified: count creatures from origin island lineage
            count++;  // This is a simplification
        }
    });

    // Return approximate population based on growth curve
    // This is a placeholder for actual species tracking
    float timeSinceArrival = record.currentTime - record.arrivalTime;
    float growthFactor = std::min(10.0f, 1.0f + timeSinceArrival * 0.01f);

    return static_cast<int>(record.founderCount * growthFactor *
                            (record.established ? 1.0f : 0.5f));
}

int InvasiveSpecies::countNativePopulation(uint32_t islandIndex,
                                            const MultiIslandManager& islands) const {
    const auto* island = islands.getIsland(islandIndex);
    if (!island) return 0;

    int total = island->stats.totalCreatures;

    // Subtract invasive populations
    for (const auto& record : m_invasionHistory) {
        if (record.targetIsland == islandIndex &&
            record.currentPhase != InvasionPhase::EXTINCTION) {
            total -= record.currentPopulation;
        }
    }

    return std::max(0, total);
}

// ============================================================================
// Impact Calculations
// ============================================================================

float InvasiveSpecies::calculateCompetitionImpact(const InvasionRecord& record,
                                                   const MultiIslandManager& islands) const {
    const auto* island = islands.getIsland(record.targetIsland);
    if (!island) return 0.0f;

    // Competition increases with population ratio
    int totalPop = island->stats.totalCreatures;
    if (totalPop == 0) return 0.0f;

    float ratio = static_cast<float>(record.currentPopulation) / totalPop;
    return std::min(1.0f, ratio * 2.0f);  // Scale up for visibility
}

float InvasiveSpecies::calculatePredationImpact(const InvasionRecord& record,
                                                 const MultiIslandManager& islands) const {
    // Simplified predation impact calculation
    // Would need to know if invader is predatory

    const auto* island = islands.getIsland(record.targetIsland);
    if (!island) return 0.0f;

    // For now, assume some predation if population is large
    if (record.currentPopulation > m_establishmentThreshold * 3) {
        return 0.3f;
    }

    return 0.0f;
}

// ============================================================================
// Alerts
// ============================================================================

void InvasiveSpecies::emitAlert(InvasionAlert::Severity severity, uint32_t invasionId,
                                 const std::string& message) {
    InvasionAlert alert;
    alert.severity = severity;
    alert.invasionId = invasionId;
    alert.message = message;
    alert.timestamp = m_totalTime;

    m_recentAlerts.push_back(alert);

    // Trim old alerts
    while (m_recentAlerts.size() > MAX_ALERTS) {
        m_recentAlerts.erase(m_recentAlerts.begin());
    }
}

} // namespace Forge
