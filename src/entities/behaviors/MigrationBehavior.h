#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>
#include "../CreatureType.h"

// Forward declarations
class Creature;
class SeasonManager;
class BiomeSystem;
class Terrain;
enum class Season;
enum class BiomeType : uint8_t;
namespace Forge { class CreatureManager; }
using Forge::CreatureManager;

/**
 * @brief Manages seasonal and resource-driven migration behavior
 *
 * Creatures migrate in response to:
 * - Seasonal changes (avoid harsh winters, follow food)
 * - Resource depletion (find better feeding grounds)
 * - Environmental stress (temperature, drought)
 * - Breeding requirements (return to ancestral grounds)
 */
class MigrationBehavior {
public:
    enum class MigrationTrigger {
        NONE,
        SEASONAL,           // Regular seasonal movement
        RESOURCE_SCARCITY,  // Food/water shortage
        TEMPERATURE_STRESS, // Too hot or cold
        BREEDING,           // Migration to breeding grounds
        FOLLOWING_HERD,     // Following group migration
        PREDATOR_PRESSURE   // Fleeing predator-heavy area
    };

    enum class MigrationPhase {
        NONE,
        PREPARING,      // Building energy reserves
        DEPARTING,      // Starting journey
        TRAVELING,      // In transit
        ARRIVING,       // Reaching destination
        RESTING,        // Recovering at destination
        SETTLED         // Migration complete
    };

    struct Migration {
        uint32_t creatureID = 0;
        glm::vec3 origin{0.0f};         // Where migration started
        glm::vec3 destination{0.0f};     // Target location
        glm::vec3 currentWaypoint{0.0f}; // Current intermediate target
        std::vector<glm::vec3> waypoints; // Route waypoints
        size_t waypointIndex = 0;
        MigrationTrigger trigger = MigrationTrigger::NONE;
        MigrationPhase phase = MigrationPhase::NONE;
        float startTime = 0.0f;
        float progress = 0.0f;          // 0-1 journey completion
        float urgency = 0.5f;           // How critical migration is
        float distanceRemaining = 0.0f;
        bool arrived = false;
    };

    struct MigrationRoute {
        BiomeType sourceBiome;
        BiomeType destBiome;
        Season triggerSeason;
        float priority;                 // Higher = more likely to take
    };

    struct MigrationConfig {
        float seasonalTriggerThreshold = 0.8f;  // Season progress to trigger
        float resourceTriggerThreshold = 0.3f;  // Resource level to trigger
        float temperatureStressThreshold = 0.7f; // Stress level to trigger
        float minEnergyToMigrate = 80.0f;       // Energy needed to start
        float migrationSpeed = 1.3f;            // Speed multiplier during travel
        float waypointReachDistance = 10.0f;    // Distance to consider waypoint reached
        float flockingDistance = 30.0f;         // Distance for migration flocking
        float restDuration = 30.0f;             // Rest time at destination
        float migrationCooldown = 120.0f;       // Min time between migrations
    };

    MigrationBehavior() = default;
    ~MigrationBehavior() = default;

    /**
     * @brief Initialize with references to dependent systems
     */
    void init(SeasonManager* seasons, BiomeSystem* biomes, Terrain* terrain);

    /**
     * @brief Update all migrations - called once per frame
     */
    void update(float deltaTime, CreatureManager& creatures);

    /**
     * @brief Calculate migration steering force for a creature
     */
    glm::vec3 calculateForce(Creature* creature);

    /**
     * @brief Check if a creature is currently migrating
     */
    bool isMigrating(uint32_t creatureID) const;

    /**
     * @brief Get migration info for a creature
     */
    const Migration* getMigration(uint32_t creatureID) const;

    /**
     * @brief Force a creature to start migration
     */
    bool startMigration(uint32_t creatureID, const glm::vec3& destination,
                        MigrationTrigger trigger = MigrationTrigger::SEASONAL);

    /**
     * @brief Cancel an active migration
     */
    void cancelMigration(uint32_t creatureID);

    /**
     * @brief Get all active migrations for visualization
     */
    const std::unordered_map<uint32_t, Migration>& getActiveMigrations() const {
        return m_activeMigrations;
    }

    /**
     * @brief Find suitable migration destination for a creature
     */
    glm::vec3 findMigrationDestination(Creature* creature, MigrationTrigger trigger);

    /**
     * @brief Configuration access
     */
    MigrationConfig& getConfig() { return m_config; }
    const MigrationConfig& getConfig() const { return m_config; }

    /**
     * @brief Statistics
     */
    size_t getActiveMigrationCount() const { return m_activeMigrations.size(); }
    int getCompletedMigrations() const { return m_completedMigrations; }

    /**
     * @brief Check if creature type can migrate
     */
    static bool canMigrate(CreatureType type);

private:
    // Check seasonal migration triggers
    void checkSeasonalTriggers(CreatureManager& creatures);

    // Check resource-based triggers
    void checkResourceTriggers(CreatureManager& creatures);

    // Update active migrations
    void updateActiveMigrations(float deltaTime, CreatureManager& creatures);

    // Generate waypoints for a migration route
    std::vector<glm::vec3> generateWaypoints(const glm::vec3& origin,
                                             const glm::vec3& destination);

    // Find suitable biome for creature type
    glm::vec3 findSuitableBiome(Creature* creature, Season targetSeason);

    // Calculate migration priority based on conditions
    float calculateMigrationPriority(Creature* creature, MigrationTrigger trigger);

    // Check if creature should join nearby migration
    bool shouldJoinMigration(Creature* creature, const Migration& nearbyMigration);

    // Process arrival at destination
    void processArrival(Migration& migration, Creature* creature);

    std::unordered_map<uint32_t, Migration> m_activeMigrations;
    std::unordered_map<uint32_t, float> m_migrationCooldowns;  // creatureID -> time remaining
    std::unordered_set<uint32_t> m_migrationsToRemove;
    std::vector<MigrationRoute> m_knownRoutes;

    SeasonManager* m_seasonManager = nullptr;
    BiomeSystem* m_biomeSystem = nullptr;
    Terrain* m_terrain = nullptr;

    Season m_lastSeason;
    bool m_seasonChanged = false;
    MigrationConfig m_config;
    float m_currentTime = 0.0f;
    int m_completedMigrations = 0;
};
