#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <cstdint>

// Forward declarations
class Creature;
class SpatialGrid;
namespace Forge { class CreatureManager; }
using Forge::CreatureManager;

/**
 * @brief Manages territorial behavior for creatures that defend space
 *
 * Territories are established by creatures with territorial traits.
 * Owners patrol and defend their territory from same-species intruders.
 * Non-owners are repelled from established territories.
 */
class TerritorialBehavior {
public:
    struct Territory {
        uint32_t ownerID = 0;           // Creature ID that owns this territory
        glm::vec3 center{0.0f};         // Territory center (updates to track owner)
        float radius = 15.0f;           // Territory radius
        float strength = 0.0f;          // 0-1, increases over time as established
        float establishedTime = 0.0f;   // When territory was first claimed
        float resourceQuality = 1.0f;   // Resource density in territory
        int intrusionCount = 0;         // Recent intrusions (affects aggression)
        float lastDefenseTime = 0.0f;   // Last time owner defended
    };

    struct TerritorialConfig {
        float minEnergyToEstablish = 100.0f;    // Energy needed to claim territory
        float baseRadius = 15.0f;                // Default territory size
        float radiusPerSize = 5.0f;              // Additional radius per creature size
        float strengthGainRate = 0.02f;          // Strength increase per second
        float strengthDecayRate = 0.01f;         // Strength decay when away
        float defenseForceMultiplier = 2.0f;     // Force to chase intruders
        float repulsionForceMultiplier = 1.5f;   // Force pushing intruders away
        float maxTerritoryAge = 300.0f;          // Max time before territory weakens
        float abandonDistance = 2.0f;            // Distance multiplier before abandoning
        int maxIntrusionsBeforeAggression = 3;   // Intrusions before heightened defense
    };

    TerritorialBehavior() = default;
    ~TerritorialBehavior() = default;

    /**
     * @brief Update all territories - called once per frame
     * @param deltaTime Time elapsed since last update
     * @param creatures Reference to creature manager for lookups
     * @param grid Spatial grid for efficient neighbor queries
     */
    void update(float deltaTime, CreatureManager& creatures, const SpatialGrid& grid);

    /**
     * @brief Calculate territorial steering force for a creature
     * @param creature The creature to calculate force for
     * @param grid Spatial grid for neighbor queries
     * @return Steering force vector (add to creature's velocity)
     */
    glm::vec3 calculateForce(Creature* creature, const SpatialGrid& grid);

    /**
     * @brief Attempt to establish a new territory for a creature
     * @param creature The creature attempting to claim territory
     * @param terrain Used to evaluate resource quality
     * @return true if territory was successfully established
     */
    bool tryEstablishTerritory(Creature* creature, float resourceQuality = 1.0f);

    /**
     * @brief Abandon a creature's territory voluntarily
     * @param creatureID ID of the creature abandoning territory
     */
    void abandonTerritory(uint32_t creatureID);

    /**
     * @brief Check if a creature has an established territory
     * @param creatureID ID of the creature to check
     * @return true if creature owns a territory
     */
    bool hasTerritory(uint32_t creatureID) const;

    /**
     * @brief Get a creature's territory information
     * @param creatureID ID of the creature
     * @return Pointer to territory, or nullptr if none
     */
    const Territory* getTerritory(uint32_t creatureID) const;

    /**
     * @brief Check if a position is inside any territory
     * @param position World position to check
     * @param excludeOwnerID Optional owner to exclude from check
     * @return Owner ID if in territory, 0 if not
     */
    uint32_t isInTerritory(const glm::vec3& position, uint32_t excludeOwnerID = 0) const;

    /**
     * @brief Get all current territories for visualization
     * @return Read-only map of creature ID to territory
     */
    const std::unordered_map<uint32_t, Territory>& getTerritories() const { return m_territories; }

    /**
     * @brief Configuration access
     */
    TerritorialConfig& getConfig() { return m_config; }
    const TerritorialConfig& getConfig() const { return m_config; }

    /**
     * @brief Statistics for debugging/UI
     */
    size_t getTerritoryCount() const { return m_territories.size(); }
    float getAverageStrength() const;
    int getTotalIntrusions() const;

private:
    // Clean up territories for dead creatures
    void removeDeadOwnerTerritories(CreatureManager& creatures);

    // Update territory center to follow owner (with drift)
    void updateTerritoryCenter(Territory& territory, const glm::vec3& ownerPos, float deltaTime);

    // Check for intrusions and update intrusion counts
    void processIntrusions(Territory& territory, Creature* owner, const SpatialGrid& grid, float currentTime);

    // Calculate force for territory owner (chase intruders)
    glm::vec3 calculateOwnerForce(Creature* owner, const Territory& territory, const SpatialGrid& grid);

    // Calculate force for non-owner (avoid territories)
    glm::vec3 calculateIntruderForce(Creature* intruder);

    std::unordered_map<uint32_t, Territory> m_territories;
    std::vector<uint32_t> m_territoriesToRemove;  // Deferred removal list
    TerritorialConfig m_config;
    float m_currentTime = 0.0f;
};
