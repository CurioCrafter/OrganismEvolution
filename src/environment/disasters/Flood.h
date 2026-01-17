#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "../DisasterSystem.h"

// Forward declarations
namespace Forge {
    class CreatureManager;
}
class Terrain;
struct ActiveDisaster;

using Forge::CreatureManager;

namespace disasters {

/**
 * @brief Phases of a flood
 */
enum class FloodPhase {
    RISING,         // Water levels increasing
    PEAK,           // Maximum flood level
    SUSTAINED,      // High water maintained
    RECEDING,       // Water draining
    AFTERMATH       // Cleanup and recovery
};

/**
 * @brief Individual flooded area
 */
struct FloodedArea {
    glm::vec3 center;
    float currentLevel;
    float maxLevel;
    float radius;
    bool active;
};

/**
 * @brief Flood disaster handler
 *
 * Simulates flooding event with:
 * - Rising water levels
 * - Low-lying areas submerged
 * - Land creature drowning
 * - Aquatic creatures benefit
 * - Displacement and migration
 *
 * Evolutionary pressure:
 * - Favors swimming ability
 * - Selects for climbing behavior
 * - Rewards amphibious traits
 * - Creates temporary barriers
 */
class Flood {
public:
    Flood();
    ~Flood() = default;

    /**
     * @brief Trigger a new flood
     * @param epicenter Flood origin point
     * @param severity How severe the flood is
     */
    void trigger(const glm::vec3& epicenter, DisasterSeverity severity);

    /**
     * @brief Update the flood simulation
     */
    void update(float deltaTime, CreatureManager& creatures, Terrain& terrain,
                ActiveDisaster& disaster);

    /**
     * @brief Reset to inactive state
     */
    void reset();

    // === Accessors ===
    bool isActive() const { return m_active; }
    FloodPhase getCurrentPhase() const { return m_currentPhase; }
    float getProgress() const { return m_progress; }

    /**
     * @brief Get current water level at position
     */
    float getWaterLevelAt(const glm::vec3& position) const;

    /**
     * @brief Get peak water rise amount
     */
    float getPeakWaterRise() const { return m_targetWaterRise; }

    /**
     * @brief Get current water rise amount
     */
    float getCurrentWaterRise() const { return m_currentWaterRise; }

    /**
     * @brief Check if position is flooded
     */
    bool isFlooded(const glm::vec3& position, const Terrain& terrain) const;

    /**
     * @brief Get flood depth at position
     */
    float getFloodDepth(const glm::vec3& position, const Terrain& terrain) const;

    /**
     * @brief Get all flooded areas
     */
    const std::vector<FloodedArea>& getFloodedAreas() const { return m_floodedAreas; }

private:
    // === Phase Updates ===
    void updateRisingPhase(float deltaTime, ActiveDisaster& disaster);
    void updatePeakPhase(float deltaTime, ActiveDisaster& disaster);
    void updateSustainedPhase(float deltaTime, ActiveDisaster& disaster);
    void updateRecedingPhase(float deltaTime, ActiveDisaster& disaster);
    void updateAftermathPhase(float deltaTime, ActiveDisaster& disaster);

    void advancePhase(ActiveDisaster& disaster);
    void applyCreatureEffects(CreatureManager& creatures, const Terrain& terrain,
                              float deltaTime, ActiveDisaster& disaster);
    void updateFloodedAreas(float deltaTime);
    float calculateDrowningDamage(float floodDepth, float creatureEnergy, bool canSwim) const;

    // === State ===
    bool m_active = false;
    FloodPhase m_currentPhase = FloodPhase::RISING;
    DisasterSeverity m_severity = DisasterSeverity::MODERATE;
    float m_progress = 0.0f;

    // === Flood Parameters ===
    glm::vec3 m_epicenter;
    float m_targetWaterRise = 15.0f;
    float m_currentWaterRise = 0.0f;
    float m_floodRadius = 100.0f;
    float m_riseRate = 1.0f;
    float m_baseDrowningDamage = 5.0f;

    // === Flooded Areas ===
    std::vector<FloodedArea> m_floodedAreas;

    // === Timing ===
    float m_phaseTimer = 0.0f;
    float m_risingDuration = 30.0f;
    float m_peakDuration = 15.0f;
    float m_sustainedDuration = 60.0f;
    float m_recedingDuration = 45.0f;
    float m_aftermathDuration = 30.0f;

    // === Random Generation ===
    std::mt19937 m_rng;
};

} // namespace disasters
