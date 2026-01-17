#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "../DisasterSystem.h"

// Forward declarations
class VegetationManager;
namespace Forge {
    class CreatureManager;
}
class Terrain;
struct ActiveDisaster;

using Forge::CreatureManager;

namespace disasters {

/**
 * @brief Phases of a drought
 */
enum class DroughtPhase {
    DEVELOPING,     // Dry conditions begin
    SEVERE,         // Lack of water critical
    EXTREME,        // Maximum severity
    BREAKING,       // Relief begins
    RECOVERY        // Return to normal
};

/**
 * @brief Data for dried water bodies
 */
struct DriedWaterBody {
    glm::vec3 position;
    float originalDepth;
    float currentLevel;
    float shrinkRate;
};

/**
 * @brief Drought disaster handler
 *
 * Simulates prolonged dry conditions with:
 * - Water source depletion
 * - Vegetation die-off
 * - Creature dehydration damage
 * - Competition for remaining water
 * - Dust storms (visual)
 *
 * Evolutionary pressure:
 * - Selects for water efficiency
 * - Favors nocturnal behavior
 * - Rewards food storage ability
 * - Encourages migration to water
 */
class Drought {
public:
    Drought();
    ~Drought() = default;

    /**
     * @brief Trigger a new drought
     * @param severity How severe the drought is
     */
    void trigger(DisasterSeverity severity);

    /**
     * @brief Update the drought simulation
     */
    void update(float deltaTime, VegetationManager& vegetation,
                CreatureManager& creatures, Terrain& terrain, ActiveDisaster& disaster);

    /**
     * @brief Reset to inactive state
     */
    void reset();

    // === Accessors ===
    bool isActive() const { return m_active; }
    DroughtPhase getCurrentPhase() const { return m_currentPhase; }
    float getProgress() const { return m_progress; }

    /**
     * @brief Get current drought severity (0-1)
     */
    float getSeverityLevel() const { return m_currentSeverity; }

    /**
     * @brief Get water availability modifier (0-1, 0 = no water)
     */
    float getWaterAvailability() const { return 1.0f - m_currentSeverity; }

    /**
     * @brief Get vegetation health modifier
     */
    float getVegetationHealth() const { return m_vegetationHealth; }

    /**
     * @brief Check if position has available water nearby
     */
    bool hasWaterNearby(const glm::vec3& position, float searchRadius) const;

    /**
     * @brief Get dehydration damage rate at position
     */
    float getDehydrationRate(const glm::vec3& position) const;

private:
    // === Phase Updates ===
    void updateDevelopingPhase(float deltaTime, ActiveDisaster& disaster);
    void updateSeverePhase(float deltaTime, ActiveDisaster& disaster);
    void updateExtremePhase(float deltaTime, ActiveDisaster& disaster);
    void updateBreakingPhase(float deltaTime, ActiveDisaster& disaster);
    void updateRecoveryPhase(float deltaTime, ActiveDisaster& disaster);

    void advancePhase(ActiveDisaster& disaster);
    void applyVegetationEffects(VegetationManager& vegetation, float deltaTime, ActiveDisaster& disaster);
    void applyCreatureEffects(CreatureManager& creatures, float deltaTime, ActiveDisaster& disaster);
    float calculateDehydrationDamage(float energy, float distanceToWater) const;

    // === State ===
    bool m_active = false;
    DroughtPhase m_currentPhase = DroughtPhase::DEVELOPING;
    DisasterSeverity m_severity = DisasterSeverity::MODERATE;
    float m_progress = 0.0f;

    // === Drought Parameters ===
    float m_targetSeverity = 0.7f;
    float m_currentSeverity = 0.0f;
    float m_vegetationHealth = 1.0f;
    float m_baseDehydrationDamage = 2.0f;

    // === Water Sources ===
    std::vector<glm::vec3> m_waterSources; // Remaining water locations

    // === Timing ===
    float m_phaseTimer = 0.0f;
    float m_developDuration = 60.0f;
    float m_severeDuration = 90.0f;
    float m_extremeDuration = 120.0f;
    float m_breakingDuration = 45.0f;
    float m_recoveryDuration = 60.0f;

    // === Random Generation ===
    std::mt19937 m_rng;
};

} // namespace disasters
