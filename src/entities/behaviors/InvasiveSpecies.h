#pragma once

// InvasiveSpecies - Tracks and analyzes the impact of invasive species on island ecosystems
// Records invasion history, establishment success, and ecological impacts

#include "../../core/MultiIslandManager.h"
#include "InterIslandMigration.h"
#include "../../entities/genetics/Species.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>
#include <deque>

namespace Forge {

// ============================================================================
// Invasion Phase
// ============================================================================

enum class InvasionPhase {
    ARRIVAL,            // Initial arrival (1-10 individuals)
    ESTABLISHMENT,      // Population stabilizing (10-50)
    EXPANSION,          // Rapid population growth
    INTEGRATION,        // Becoming part of ecosystem
    DECLINE,            // Population decreasing
    EXTINCTION,         // Failed invasion
    DOMINANT            // Became dominant species
};

// ============================================================================
// Ecological Impact
// ============================================================================

enum class EcologicalImpact {
    MINIMAL,            // Little effect on native species
    COMPETITION,        // Competing with natives for resources
    PREDATION,          // Preying on native species
    HYBRIDIZATION,      // Interbreeding with natives
    DISPLACEMENT,       // Replacing native species
    ECOSYSTEM_CHANGE    // Fundamentally altering ecosystem
};

// ============================================================================
// Invasion Record
// ============================================================================

struct InvasionRecord {
    uint32_t id;
    uint32_t speciesId;
    std::string speciesName;

    // Origin and destination
    uint32_t originIsland;
    uint32_t targetIsland;
    std::string originIslandName;
    std::string targetIslandName;

    // Timeline
    float arrivalTime;
    float establishmentTime;
    float peakTime;
    float currentTime;

    // Population tracking
    int founderCount;           // Initial population
    int peakPopulation;         // Maximum population reached
    int currentPopulation;
    bool established;           // Successfully established?

    // Phase tracking
    InvasionPhase currentPhase;
    std::vector<std::pair<float, InvasionPhase>> phaseHistory;

    // Ecological impact
    EcologicalImpact primaryImpact;
    float impactSeverity;       // 0-1

    // Affected species
    std::vector<uint32_t> competingWith;
    std::vector<uint32_t> preyingOn;
    std::vector<uint32_t> hybridizingWith;
    int nativeSpeciesDisplaced;

    // Genetic data
    float geneticDistanceFromNatives;
    float adaptationRate;       // How quickly adapting to new environment

    InvasionRecord()
        : id(0), speciesId(0), originIsland(0), targetIsland(0),
          arrivalTime(0.0f), establishmentTime(0.0f), peakTime(0.0f), currentTime(0.0f),
          founderCount(0), peakPopulation(0), currentPopulation(0), established(false),
          currentPhase(InvasionPhase::ARRIVAL), primaryImpact(EcologicalImpact::MINIMAL),
          impactSeverity(0.0f), nativeSpeciesDisplaced(0),
          geneticDistanceFromNatives(0.0f), adaptationRate(0.0f) {}
};

// ============================================================================
// Invasion Alert
// ============================================================================

struct InvasionAlert {
    enum class Severity {
        INFO,           // New arrival
        WARNING,        // Establishing population
        CRITICAL        // Causing ecological damage
    };

    Severity severity;
    uint32_t invasionId;
    std::string message;
    float timestamp;
};

// ============================================================================
// Invasion Statistics
// ============================================================================

struct InvasionStats {
    int totalInvasions = 0;
    int successfulEstablishments = 0;
    int failedInvasions = 0;
    int ongoingInvasions = 0;
    int dominantInvaders = 0;

    float averageEstablishmentRate = 0.0f;
    float averageImpactSeverity = 0.0f;
    int totalNativeDisplacements = 0;

    // Per-island stats
    std::map<uint32_t, int> invasionsByTargetIsland;
    std::map<uint32_t, int> invasionsByOriginIsland;
};

// ============================================================================
// Invasive Species Tracker
// ============================================================================

class InvasiveSpecies {
public:
    InvasiveSpecies();
    ~InvasiveSpecies() = default;

    // ========================================================================
    // Main Interface
    // ========================================================================

    // Track a new invasion from migration event
    void trackInvasion(uint32_t creatureId, uint32_t originIsland, uint32_t targetIsland);

    // Track invasion with full data
    void trackInvasion(uint32_t speciesId, const std::string& speciesName,
                       uint32_t originIsland, uint32_t targetIsland,
                       int founderCount, float timestamp);

    // Update all invasion records
    void update(float deltaTime, MultiIslandManager& islands);

    // ========================================================================
    // Invasion Records
    // ========================================================================

    const std::vector<InvasionRecord>& getInvasionHistory() const { return m_invasionHistory; }
    const InvasionRecord* getInvasion(uint32_t id) const;

    // Get invasions by status
    std::vector<const InvasionRecord*> getActiveInvasions() const;
    std::vector<const InvasionRecord*> getEstablishedInvasions() const;
    std::vector<const InvasionRecord*> getFailedInvasions() const;

    // Get invasions by island
    std::vector<const InvasionRecord*> getInvasionsOnIsland(uint32_t islandIndex) const;
    std::vector<const InvasionRecord*> getInvasionsFromIsland(uint32_t islandIndex) const;

    // ========================================================================
    // Impact Analysis
    // ========================================================================

    // Calculate overall invasive impact on an island
    float calculateIslandInvasiveLoad(uint32_t islandIndex) const;

    // Get most impactful invasions
    std::vector<const InvasionRecord*> getMostImpactfulInvasions(int count = 5) const;

    // Check if a species is invasive on a given island
    bool isSpeciesInvasive(uint32_t speciesId, uint32_t islandIndex) const;

    // Get ecological impact summary for an island
    std::map<EcologicalImpact, int> getImpactSummary(uint32_t islandIndex) const;

    // ========================================================================
    // Alerts
    // ========================================================================

    const std::vector<InvasionAlert>& getRecentAlerts() const { return m_recentAlerts; }
    void clearAlerts() { m_recentAlerts.clear(); }

    // ========================================================================
    // Statistics
    // ========================================================================

    const InvasionStats& getStats() const { return m_stats; }
    void resetStats();

    // Success rate metrics
    float getEstablishmentSuccessRate() const;
    float getAverageTimeToEstablishment() const;
    float getAverageFounderPopulation() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    // Population thresholds
    void setEstablishmentThreshold(int population) { m_establishmentThreshold = population; }
    void setDominanceThreshold(float ratio) { m_dominanceThreshold = ratio; }

    // Impact assessment sensitivity
    void setImpactSensitivity(float sensitivity) { m_impactSensitivity = sensitivity; }

private:
    // Invasion records
    std::vector<InvasionRecord> m_invasionHistory;
    uint32_t m_nextInvasionId = 1;

    // Alerts
    std::vector<InvasionAlert> m_recentAlerts;
    static constexpr size_t MAX_ALERTS = 50;

    // Statistics
    InvasionStats m_stats;

    // Configuration
    int m_establishmentThreshold = 20;      // Minimum population to be "established"
    float m_dominanceThreshold = 0.3f;      // % of island population to be "dominant"
    float m_impactSensitivity = 1.0f;       // Multiplier for impact calculations

    // Accumulated simulation time
    float m_totalTime = 0.0f;

    // Internal methods
    void updateInvasionPhase(InvasionRecord& record, MultiIslandManager& islands);
    void assessEcologicalImpact(InvasionRecord& record, MultiIslandManager& islands);
    void checkForDisplacement(InvasionRecord& record, MultiIslandManager& islands);
    void updateStatistics();

    // Population tracking
    int countInvasivePopulation(const InvasionRecord& record, const MultiIslandManager& islands) const;
    int countNativePopulation(uint32_t islandIndex, const MultiIslandManager& islands) const;

    // Impact helpers
    float calculateCompetitionImpact(const InvasionRecord& record, const MultiIslandManager& islands) const;
    float calculatePredationImpact(const InvasionRecord& record, const MultiIslandManager& islands) const;

    // Alerts
    void emitAlert(InvasionAlert::Severity severity, uint32_t invasionId, const std::string& message);
};

} // namespace Forge
