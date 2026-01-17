#pragma once

// InterIslandMigration - Handles creature travel between islands in an archipelago
// Implements various migration triggers and pathways

#include "../../core/CreatureManager.h"
#include "../../environment/ArchipelagoGenerator.h"
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <functional>

namespace Forge {

class MultiIslandManager;

// ============================================================================
// Migration Types
// ============================================================================

enum class MigrationType {
    COASTAL_DRIFT,      // Creature swept by currents while swimming
    FLYING,             // Flying creature crosses open water
    FLOATING_DEBRIS,    // Rafting on vegetation/debris
    SEASONAL,           // Triggered by environmental cues
    POPULATION_PRESSURE,// Overcrowding triggers emigration
    FOOD_SCARCITY,      // Low resources trigger search for new habitat
    MATE_SEEKING,       // Looking for mates on other islands
    RANDOM_DISPERSAL    // Chance event
};

// ============================================================================
// Migration State
// ============================================================================

enum class MigrationState {
    INITIATING,         // Just started migration attempt
    IN_TRANSIT,         // Crossing open water
    ARRIVING,           // Reaching destination
    COMPLETED,          // Successfully arrived
    FAILED              // Did not survive crossing
};

// ============================================================================
// Migration Event
// ============================================================================

struct MigrationEvent {
    uint32_t creatureId;
    uint32_t sourceIsland;
    uint32_t targetIsland;
    MigrationType type;
    MigrationState state;

    // Progress tracking
    float progress;             // 0-1, how far through the journey
    float totalDistance;        // Distance to travel
    float timeElapsed;          // Time in migration
    float estimatedDuration;    // Expected total time

    // Creature state during migration
    float startEnergy;
    float currentEnergy;
    float survivalChance;       // Probability of successful arrival

    // Path information
    glm::vec3 startPosition;
    glm::vec3 currentPosition;
    glm::vec3 targetPosition;
    glm::vec2 currentVelocity;

    // Creature data (for transfer)
    Genome genome;
    CreatureType creatureType;

    MigrationEvent()
        : creatureId(0), sourceIsland(0), targetIsland(0),
          type(MigrationType::RANDOM_DISPERSAL), state(MigrationState::INITIATING),
          progress(0.0f), totalDistance(0.0f), timeElapsed(0.0f), estimatedDuration(0.0f),
          startEnergy(0.0f), currentEnergy(0.0f), survivalChance(0.5f),
          creatureType(CreatureType::HERBIVORE) {}
};

// ============================================================================
// Migration Statistics
// ============================================================================

struct MigrationStats {
    int totalAttempts = 0;
    int successfulMigrations = 0;
    int failedMigrations = 0;
    int inProgressMigrations = 0;

    // By type
    std::array<int, 8> attemptsByType{};    // Indexed by MigrationType
    std::array<int, 8> successesByType{};

    // By island pair
    std::map<std::pair<uint32_t, uint32_t>, int> migrationsBetweenIslands;

    // Averages
    float avgSurvivalRate = 0.0f;
    float avgTravelTime = 0.0f;

    void reset() {
        totalAttempts = successfulMigrations = failedMigrations = inProgressMigrations = 0;
        attemptsByType.fill(0);
        successesByType.fill(0);
        migrationsBetweenIslands.clear();
        avgSurvivalRate = avgTravelTime = 0.0f;
    }
};

// ============================================================================
// Migration Configuration
// ============================================================================

struct MigrationConfig {
    // Base probabilities (per creature per update)
    float baseMigrationChance = 0.0001f;    // Very rare by default
    float coastalProximityBonus = 3.0f;     // Multiplier for creatures near coast
    float populationPressureThreshold = 0.8f;// Population % that triggers pressure

    // Survival factors
    float baseSwimSurvival = 0.3f;          // Base survival for swimming creatures
    float baseFlyingSurvival = 0.7f;        // Base survival for flying creatures
    float baseRaftingSurvival = 0.2f;       // Base survival for rafting

    // Energy costs
    float swimEnergyPerUnit = 0.5f;         // Energy per distance unit swimming
    float flyingEnergyPerUnit = 0.3f;       // Energy per distance unit flying
    float raftingEnergyPerUnit = 0.1f;      // Passive, lower cost

    // Speed factors (units per second)
    float swimSpeed = 5.0f;
    float flyingSpeed = 15.0f;
    float raftingSpeed = 2.0f;

    // Environmental effects
    float currentSpeedBonus = 1.5f;         // Bonus from favorable currents
    float windSpeedBonus = 1.3f;            // Bonus from favorable winds

    // Fitness effects
    float fitnessSwimBonus = 0.2f;          // Additional survival per fitness unit
    float fitnessFlyBonus = 0.1f;

    // Special triggers
    float overcrowdingThreshold = 0.9f;     // Island capacity to trigger emigration
    float starvationThreshold = 0.3f;       // Energy level to seek new habitat
};

// ============================================================================
// Inter-Island Migration System
// ============================================================================

class InterIslandMigration {
public:
    using MigrationCallback = std::function<void(const MigrationEvent&)>;

    InterIslandMigration();
    ~InterIslandMigration() = default;

    // ========================================================================
    // Main Update
    // ========================================================================

    void update(float deltaTime, MultiIslandManager& islands);

    // ========================================================================
    // Manual Migration Triggers
    // ========================================================================

    // Attempt migration for specific creature
    bool attemptMigration(uint32_t sourceIsland, CreatureHandle handle,
                          uint32_t targetIsland, MigrationType type,
                          MultiIslandManager& islands);

    // Force migration (for testing/scenarios)
    bool forceMigration(uint32_t sourceIsland, CreatureHandle handle,
                        uint32_t targetIsland, MultiIslandManager& islands);

    // ========================================================================
    // Active Migrations
    // ========================================================================

    const std::vector<MigrationEvent>& getActiveMigrations() const { return m_activeMigrations; }
    int getActiveMigrationCount() const { return static_cast<int>(m_activeMigrations.size()); }

    // Get migrations involving specific island
    std::vector<const MigrationEvent*> getMigrationsFrom(uint32_t islandIndex) const;
    std::vector<const MigrationEvent*> getMigrationsTo(uint32_t islandIndex) const;

    // ========================================================================
    // Configuration
    // ========================================================================

    void setConfig(const MigrationConfig& config) { m_config = config; }
    const MigrationConfig& getConfig() const { return m_config; }

    // Enable/disable migration types
    void setMigrationTypeEnabled(MigrationType type, bool enabled);
    bool isMigrationTypeEnabled(MigrationType type) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    const MigrationStats& getStats() const { return m_stats; }
    void resetStats() { m_stats.reset(); }

    // ========================================================================
    // Callbacks
    // ========================================================================

    void registerCallback(MigrationCallback callback);

    // ========================================================================
    // Utility
    // ========================================================================

    // Calculate survival chance for a migration
    float calculateSurvivalChance(const Creature* creature, uint32_t sourceIsland,
                                   uint32_t targetIsland, MigrationType type,
                                   const MultiIslandManager& islands) const;

    // Calculate travel time
    float calculateTravelTime(uint32_t sourceIsland, uint32_t targetIsland,
                              MigrationType type, const MultiIslandManager& islands) const;

    // Get best migration type for a creature
    MigrationType getBestMigrationType(const Creature* creature);

private:
    // Active migrations
    std::vector<MigrationEvent> m_activeMigrations;

    // Configuration
    MigrationConfig m_config;
    std::array<bool, 8> m_enabledTypes;

    // Statistics
    MigrationStats m_stats;

    // Callbacks
    std::vector<MigrationCallback> m_callbacks;

    // Random number generator
    std::mt19937 m_rng;

    // Internal methods
    void checkMigrationTriggers(MultiIslandManager& islands);
    void processMigrations(float deltaTime, MultiIslandManager& islands);
    void completeMigration(MigrationEvent& event, MultiIslandManager& islands);
    void failMigration(MigrationEvent& event, MultiIslandManager& islands);

    // Trigger detection
    bool checkCoastalDrift(const Creature* creature, uint32_t islandIndex,
                           const MultiIslandManager& islands);
    bool checkPopulationPressure(uint32_t islandIndex, const MultiIslandManager& islands);
    bool checkFoodScarcity(const Creature* creature, uint32_t islandIndex,
                           const MultiIslandManager& islands);

    // Target selection
    uint32_t selectTargetIsland(uint32_t sourceIsland, const Creature* creature,
                                 MigrationType type, const MultiIslandManager& islands);

    // Position calculation
    glm::vec3 calculateArrivalPosition(uint32_t targetIsland, MigrationType type,
                                        const MultiIslandManager& islands);
    glm::vec3 interpolateMigrationPosition(const MigrationEvent& event) const;

    // Survival calculation helpers
    float getTypeBaseSurvival(MigrationType type) const;
    float getTypeSpeed(MigrationType type) const;
    float getTypeEnergyCost(MigrationType type) const;

    // Notify callbacks
    void notifyCallbacks(const MigrationEvent& event);
};

} // namespace Forge
