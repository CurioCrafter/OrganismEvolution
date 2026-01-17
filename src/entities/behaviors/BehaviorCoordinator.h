#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <cstdint>

// Include behavior systems
#include "TerritorialBehavior.h"
#include "SocialGroups.h"
#include "PackHunting.h"
#include "MigrationBehavior.h"
#include "ParentalCare.h"
#include "VarietyBehaviors.h"

// Forward declarations
class Creature;
class SpatialGrid;
class FoodChainManager;
class SeasonManager;
class BiomeSystem;
class Terrain;
// CreatureManager is already forward declared via included behavior headers

/**
 * @brief Central coordinator for all emergent creature behavior systems
 *
 * This class unifies territorial, social, hunting, migration, and parental behaviors.
 * It manages behavior priorities, conflict resolution, and force blending.
 */
class BehaviorCoordinator {
public:
    /**
     * @brief Priority weights for different behavior types
     */
    struct BehaviorWeights {
        float territorial = 1.0f;
        float social = 0.8f;
        float hunting = 1.5f;
        float migration = 2.0f;
        float parental = 1.2f;
        float fleeFromPredator = 3.0f;  // Highest priority - survival
    };

    /**
     * @brief Statistics for debugging and UI
     */
    struct BehaviorStats {
        // Territorial
        size_t territoryCount = 0;
        float avgTerritoryStrength = 0.0f;
        int totalIntrusions = 0;

        // Social
        size_t groupCount = 0;
        size_t avgGroupSize = 0;
        size_t largestGroup = 0;

        // Hunting
        size_t activeHunts = 0;
        int successfulHunts = 0;
        int failedHunts = 0;

        // Migration
        size_t activeMigrations = 0;
        int completedMigrations = 0;

        // Parental
        size_t parentChildBonds = 0;
        float avgBondStrength = 0.0f;
        float totalEnergyShared = 0.0f;

        // Variety behaviors (Phase 7)
        int curiosityBehaviors = 0;
        int matingDisplays = 0;
        int scavengingBehaviors = 0;
        int playBehaviors = 0;
        int varietyTransitions = 0;
    };

    BehaviorCoordinator();
    ~BehaviorCoordinator() = default;

    /**
     * @brief Initialize with references to required systems
     */
    void init(CreatureManager* creatureManager,
              SpatialGrid* spatialGrid,
              FoodChainManager* foodChain,
              SeasonManager* seasonManager,
              BiomeSystem* biomeSystem,
              Terrain* terrain);

    /**
     * @brief Reset all behavior systems (for new simulation)
     */
    void reset();

    /**
     * @brief Main update - updates all behavior systems
     */
    void update(float deltaTime);

    /**
     * @brief Calculate combined behavior forces for a creature
     * @param creature The creature to calculate forces for
     * @return Combined steering force from all applicable behaviors
     */
    glm::vec3 calculateBehaviorForces(Creature* creature);

    /**
     * @brief Register a birth event for parental care tracking
     */
    void registerBirth(Creature* parent, Creature* child);

    /**
     * @brief Try to establish a territory for a creature
     */
    bool tryEstablishTerritory(Creature* creature, float resourceQuality = 1.0f);

    /**
     * @brief Check if creature is in an active behavior state
     */
    bool isInActiveBehavior(uint32_t creatureID) const;

    /**
     * @brief Get behavior state description for a creature (for UI)
     */
    std::string getBehaviorState(uint32_t creatureID) const;

    // Direct access to subsystems (for advanced queries/visualization)
    TerritorialBehavior& getTerritorialBehavior() { return m_territorialBehavior; }
    const TerritorialBehavior& getTerritorialBehavior() const { return m_territorialBehavior; }

    SocialGroupManager& getSocialGroups() { return m_socialGroups; }
    const SocialGroupManager& getSocialGroups() const { return m_socialGroups; }

    PackHuntingBehavior& getPackHunting() { return m_packHunting; }
    const PackHuntingBehavior& getPackHunting() const { return m_packHunting; }

    MigrationBehavior& getMigration() { return m_migration; }
    const MigrationBehavior& getMigration() const { return m_migration; }

    ParentalCareBehavior& getParentalCare() { return m_parentalCare; }
    const ParentalCareBehavior& getParentalCare() const { return m_parentalCare; }

    VarietyBehaviorManager& getVarietyBehaviors() { return m_varietyBehaviors; }
    const VarietyBehaviorManager& getVarietyBehaviors() const { return m_varietyBehaviors; }

    // Weight configuration
    BehaviorWeights& getWeights() { return m_weights; }
    const BehaviorWeights& getWeights() const { return m_weights; }

    // Statistics
    BehaviorStats getStats() const;

    /**
     * @brief Enable/disable individual behavior systems
     */
    void setTerritorialEnabled(bool enabled) { m_territorialEnabled = enabled; }
    void setSocialEnabled(bool enabled) { m_socialEnabled = enabled; }
    void setHuntingEnabled(bool enabled) { m_huntingEnabled = enabled; }
    void setMigrationEnabled(bool enabled) { m_migrationEnabled = enabled; }
    void setParentalEnabled(bool enabled) { m_parentalEnabled = enabled; }
    void setVarietyEnabled(bool enabled) { m_varietyEnabled = enabled; }

    bool isTerritorialEnabled() const { return m_territorialEnabled; }
    bool isSocialEnabled() const { return m_socialEnabled; }
    bool isHuntingEnabled() const { return m_huntingEnabled; }
    bool isMigrationEnabled() const { return m_migrationEnabled; }
    bool isParentalEnabled() const { return m_parentalEnabled; }
    bool isVarietyEnabled() const { return m_varietyEnabled; }

    /**
     * @brief Notify of creature death for scavenging behavior
     */
    void onCreatureDeath(uint32_t creatureId, const glm::vec3& deathPos);

    /**
     * @brief Enable debug logging for variety behaviors
     */
    void setVarietyDebugLogging(bool enabled) { m_varietyBehaviors.setDebugLogging(enabled); }

private:
    // Calculate flee force from nearby predators (survival behavior)
    glm::vec3 calculateFleeForce(Creature* creature);

    // Resolve conflicts between behaviors (e.g., migration vs territory)
    glm::vec3 resolveConflicts(Creature* creature, const glm::vec3& territorial,
                               const glm::vec3& social, const glm::vec3& hunting,
                               const glm::vec3& migration, const glm::vec3& parental,
                               const glm::vec3& flee);

    // Behavior subsystems
    TerritorialBehavior m_territorialBehavior;
    SocialGroupManager m_socialGroups;
    PackHuntingBehavior m_packHunting;
    MigrationBehavior m_migration;
    ParentalCareBehavior m_parentalCare;
    VarietyBehaviorManager m_varietyBehaviors;

    // External system references
    CreatureManager* m_creatureManager = nullptr;
    SpatialGrid* m_spatialGrid = nullptr;
    FoodChainManager* m_foodChain = nullptr;
    SeasonManager* m_seasonManager = nullptr;
    BiomeSystem* m_biomeSystem = nullptr;
    Terrain* m_terrain = nullptr;

    // Configuration
    BehaviorWeights m_weights;
    bool m_territorialEnabled = true;
    bool m_socialEnabled = true;
    bool m_huntingEnabled = true;
    bool m_migrationEnabled = true;
    bool m_parentalEnabled = true;
    bool m_varietyEnabled = true;

    // State
    float m_currentTime = 0.0f;
    bool m_initialized = false;
};
