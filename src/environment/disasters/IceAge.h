#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "../DisasterSystem.h"

// Forward declarations
class ClimateSystem;
class VegetationManager;
namespace Forge {
    class CreatureManager;
}
struct ActiveDisaster;

using Forge::CreatureManager;

namespace disasters {

/**
 * @brief Phases of an ice age
 */
enum class IceAgePhase {
    ONSET,          // Cooling begins
    PEAK,           // Maximum cold
    PLATEAU,        // Sustained cold
    THAW,           // Warming begins
    RECOVERY        // Return to normal
};

/**
 * @brief Glacier data for visualization
 */
struct Glacier {
    glm::vec3 position;
    float size;
    float growthRate;
    bool active;
};

/**
 * @brief Ice age disaster handler
 *
 * Simulates global cooling event with:
 * - Gradual temperature decrease
 * - Glacier formation at high altitudes
 * - Vegetation die-off in cold regions
 * - Migration pressure toward equator
 * - Favors cold-adapted creatures
 *
 * Evolutionary pressure:
 * - Selects for cold tolerance
 * - Favors larger body size (Bergmann's rule)
 * - Rewards efficient metabolism
 * - Creates geographic isolation
 */
class IceAge {
public:
    IceAge();
    ~IceAge() = default;

    /**
     * @brief Trigger a new ice age
     * @param severity How severe the cooling is
     */
    void trigger(DisasterSeverity severity);

    /**
     * @brief Update the ice age simulation
     */
    void update(float deltaTime, ClimateSystem& climate, VegetationManager& vegetation,
                CreatureManager& creatures, ActiveDisaster& disaster);

    /**
     * @brief Reset to inactive state
     */
    void reset();

    // === Accessors ===
    bool isActive() const { return m_active; }
    IceAgePhase getCurrentPhase() const { return m_currentPhase; }
    float getProgress() const { return m_progress; }

    /**
     * @brief Get current temperature modifier (negative)
     */
    float getTemperatureModifier() const { return m_currentTempModifier; }

    /**
     * @brief Get snow/ice coverage (0-1)
     */
    float getIceCoverage() const { return m_iceCoverage; }

    /**
     * @brief Check if a position is glaciated
     */
    bool isGlaciated(const glm::vec3& position) const;

    /**
     * @brief Get glaciers for visualization
     */
    const std::vector<Glacier>& getGlaciers() const { return m_glaciers; }

    /**
     * @brief Get vegetation reduction factor (0-1, 1 = no vegetation)
     */
    float getVegetationReduction() const { return m_vegetationReduction; }

private:
    // === Phase Updates ===
    void updateOnsetPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);
    void updatePeakPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);
    void updatePlateauPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);
    void updateThawPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);
    void updateRecoveryPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);

    void advancePhase(ActiveDisaster& disaster);
    void updateGlaciers(float deltaTime);
    void applyVegetationEffects(VegetationManager& vegetation, float deltaTime, ActiveDisaster& disaster);
    void applyCreatureEffects(CreatureManager& creatures, float deltaTime, ActiveDisaster& disaster);
    float calculateColdDamage(float temperature, float creatureEnergy) const;

    // === State ===
    bool m_active = false;
    IceAgePhase m_currentPhase = IceAgePhase::ONSET;
    DisasterSeverity m_severity = DisasterSeverity::MODERATE;
    float m_progress = 0.0f;

    // === Temperature ===
    float m_targetTempModifier = -20.0f;
    float m_currentTempModifier = 0.0f;
    float m_peakCooling = -25.0f;

    // === Effects ===
    float m_iceCoverage = 0.0f;
    float m_vegetationReduction = 0.0f;
    float m_glacierLine = 50.0f;    // Altitude above which glaciers form

    // === Glaciers ===
    std::vector<Glacier> m_glaciers;

    // === Timing ===
    float m_phaseTimer = 0.0f;
    float m_onsetDuration = 60.0f;
    float m_peakDuration = 30.0f;
    float m_plateauDuration = 300.0f;
    float m_thawDuration = 60.0f;
    float m_recoveryDuration = 60.0f;

    // === Random Generation ===
    std::mt19937 m_rng;
};

} // namespace disasters
