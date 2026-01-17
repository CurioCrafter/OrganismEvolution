#pragma once

// DarwinMode - Special simulation mode that emphasizes and tracks divergent evolution
// Inspired by Darwin's observations on the Galapagos Islands
// Tracks species divergence, adaptive radiation, and founder effects

#include "../core/MultiIslandManager.h"
#include "../entities/behaviors/InterIslandMigration.h"
#include "../entities/behaviors/InvasiveSpecies.h"
#include "../entities/genetics/Species.h"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>

namespace Forge {

// ============================================================================
// Divergence Data
// ============================================================================

struct DivergenceData {
    uint32_t islandA;
    uint32_t islandB;

    // Genetic metrics
    float geneticDistance;
    float morphologicalDivergence;
    float behavioralDivergence;

    // Trend data
    float geneticDistanceTrend;      // Positive = diverging, negative = converging
    float divergenceRate;            // Distance change per generation

    // Time tracking
    float timeSinceSplit;
    int generationsSinceSplit;

    // Isolation data
    float reproductiveIsolation;     // 0-1, how isolated the populations are
    bool speciationComplete;         // Have they become separate species?

    DivergenceData()
        : islandA(0), islandB(0), geneticDistance(0.0f),
          morphologicalDivergence(0.0f), behavioralDivergence(0.0f),
          geneticDistanceTrend(0.0f), divergenceRate(0.0f),
          timeSinceSplit(0.0f), generationsSinceSplit(0),
          reproductiveIsolation(0.0f), speciationComplete(false) {}
};

// ============================================================================
// Adaptive Radiation Event
// ============================================================================

struct AdaptiveRadiationEvent {
    uint32_t ancestorSpeciesId;
    std::vector<uint32_t> descendantSpeciesIds;
    uint32_t originIsland;
    std::vector<uint32_t> colonizedIslands;

    float startTime;
    int startGeneration;
    int currentGeneration;

    int nichesFilled;
    std::vector<std::string> niches;

    bool isActive;
    float radiationRate;  // Species/generation

    AdaptiveRadiationEvent()
        : ancestorSpeciesId(0), originIsland(0), startTime(0.0f),
          startGeneration(0), currentGeneration(0), nichesFilled(0),
          isActive(true), radiationRate(0.0f) {}
};

// ============================================================================
// Founder Effect Record
// ============================================================================

struct FounderEffectRecord {
    uint32_t islandIndex;
    std::string islandName;
    float colonizationTime;
    int founderCount;

    // Genetic snapshot at founding
    float initialGeneticDiversity;
    float currentGeneticDiversity;

    // Bottleneck effects
    float bottleneckSeverity;
    float recoveryProgress;

    // Unique traits that emerged
    std::vector<std::string> uniqueTraits;
    float divergenceFromAncestor;

    FounderEffectRecord()
        : islandIndex(0), colonizationTime(0.0f), founderCount(0),
          initialGeneticDiversity(0.0f), currentGeneticDiversity(0.0f),
          bottleneckSeverity(0.0f), recoveryProgress(0.0f),
          divergenceFromAncestor(0.0f) {}
};

// ============================================================================
// Darwin Mode Configuration
// ============================================================================

struct DarwinModeConfig {
    // Speciation thresholds
    float speciationGeneticThreshold = 0.7f;     // Genetic distance for speciation
    float speciationIsolationThreshold = 0.8f;   // Reproductive isolation for speciation

    // Divergence tracking
    float divergenceUpdateInterval = 5.0f;       // Seconds between divergence updates
    int historyLength = 100;                     // Generations of history to keep

    // Founder effect settings
    int founderBottleneckSize = 10;              // Population considered a founder event
    float founderDiversityThreshold = 0.3f;      // Diversity loss to count as bottleneck

    // Adaptive radiation settings
    int radiationNicheThreshold = 3;             // Niches for radiation event
    float radiationRateThreshold = 0.1f;         // Species/gen for active radiation

    // Simulation modifiers
    float mutationRateModifier = 1.0f;           // Increase/decrease mutation rates
    float migrationRateModifier = 0.5f;          // Reduce migration for more divergence
    float selectionPressureModifier = 1.2f;      // Stronger selection
};

// ============================================================================
// Darwin Mode
// ============================================================================

class DarwinMode {
public:
    DarwinMode();
    ~DarwinMode() = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    void init(MultiIslandManager& islands);
    void reset();

    // ========================================================================
    // Update
    // ========================================================================

    void update(float deltaTime);

    // ========================================================================
    // Divergence Data
    // ========================================================================

    const std::vector<DivergenceData>& getDivergenceData() const { return m_divergenceData; }

    // Get divergence between specific islands
    const DivergenceData* getDivergence(uint32_t islandA, uint32_t islandB) const;

    // Get most divergent pairs
    std::vector<const DivergenceData*> getMostDivergent(int count = 5) const;

    // Get pairs approaching speciation
    std::vector<const DivergenceData*> getNearSpeciation(float threshold = 0.6f) const;

    // ========================================================================
    // Adaptive Radiation
    // ========================================================================

    const std::vector<AdaptiveRadiationEvent>& getAdaptiveRadiations() const {
        return m_adaptiveRadiations;
    }

    bool isAdaptiveRadiationOccurring() const;
    int getActiveRadiationCount() const;

    // ========================================================================
    // Founder Effects
    // ========================================================================

    const std::vector<FounderEffectRecord>& getFounderEffects() const {
        return m_founderEffects;
    }

    // Get islands showing strong founder effects
    std::vector<const FounderEffectRecord*> getStrongFounderEffects(float threshold = 0.5f) const;

    // ========================================================================
    // Summary Statistics
    // ========================================================================

    struct DarwinSummary {
        int totalIslands = 0;
        int islandsWithUniqueSpecies = 0;
        float averageGeneticDistance = 0.0f;
        float maxGeneticDistance = 0.0f;
        int activeSpeciationEvents = 0;
        int completedSpeciations = 0;
        int adaptiveRadiations = 0;
        int founderEvents = 0;
        float overallDivergenceRate = 0.0f;
    };

    DarwinSummary getSummary() const;

    // ========================================================================
    // Configuration
    // ========================================================================

    void setConfig(const DarwinModeConfig& config) { m_config = config; }
    const DarwinModeConfig& getConfig() const { return m_config; }

    // Enable/disable Darwin mode effects
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // ========================================================================
    // Access to Systems
    // ========================================================================

    void setMigrationSystem(InterIslandMigration* migration) { m_migration = migration; }
    void setInvasiveTracker(InvasiveSpecies* invasive) { m_invasiveTracker = invasive; }

private:
    // References to managed systems
    MultiIslandManager* m_islands = nullptr;
    InterIslandMigration* m_migration = nullptr;
    InvasiveSpecies* m_invasiveTracker = nullptr;

    // Configuration
    DarwinModeConfig m_config;
    bool m_enabled = true;

    // Divergence tracking
    std::vector<DivergenceData> m_divergenceData;
    std::map<std::pair<uint32_t, uint32_t>, std::vector<float>> m_divergenceHistory;

    // Adaptive radiation tracking
    std::vector<AdaptiveRadiationEvent> m_adaptiveRadiations;

    // Founder effect tracking
    std::vector<FounderEffectRecord> m_founderEffects;

    // Timing
    float m_totalTime = 0.0f;
    float m_timeSinceLastUpdate = 0.0f;
    int m_currentGeneration = 0;

    // Internal methods
    void updateDivergenceData();
    void updateAdaptiveRadiations();
    void updateFounderEffects();
    void checkForSpeciation();
    void checkForNewRadiation();

    // Calculation helpers
    float calculateGeneticDistance(uint32_t islandA, uint32_t islandB) const;
    float calculateMorphologicalDivergence(uint32_t islandA, uint32_t islandB) const;
    float calculateBehavioralDivergence(uint32_t islandA, uint32_t islandB) const;
    float calculateReproductiveIsolation(uint32_t islandA, uint32_t islandB) const;

    // Trait analysis
    std::vector<std::string> identifyUniqueTraits(uint32_t islandIndex) const;
    std::vector<std::string> identifyNiches(const std::vector<uint32_t>& speciesIds) const;

    // History management
    void recordDivergenceHistory(uint32_t islandA, uint32_t islandB, float distance);
    float calculateDivergenceTrend(uint32_t islandA, uint32_t islandB) const;
};

} // namespace Forge
