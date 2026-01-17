#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "../DisasterSystem.h"

// Forward declarations
namespace Forge {
    class CreatureManager;
}
class VegetationManager;
class Terrain;
struct ActiveDisaster;

using Forge::CreatureManager;

namespace disasters {

/**
 * @brief Lava flow particle for visual effects
 */
struct LavaParticle {
    glm::vec3 position;
    glm::vec3 velocity;
    float temperature;      // 800-1200 degrees C
    float lifetime;
    float maxLifetime;
    float size;
    glm::vec3 color;        // Orange to red gradient
    bool active;
};

/**
 * @brief Represents a flowing lava stream
 */
struct LavaFlow {
    glm::vec3 origin;
    glm::vec3 currentPosition;
    glm::vec3 direction;
    float temperature;
    float width;
    float length;
    float speed;
    std::vector<glm::vec3> pathPoints;
    bool active;
};

/**
 * @brief Pyroclastic flow (deadly hot gas/debris cloud)
 */
struct PyroclasticFlow {
    glm::vec3 origin;
    glm::vec3 currentPosition;
    glm::vec3 direction;
    float radius;
    float speed;
    float temperature;      // 300-700 degrees C
    bool active;
};

/**
 * @brief Volcanic ash cloud for atmospheric effects
 */
struct AshCloud {
    glm::vec3 position;
    float radius;
    float density;          // 0-1
    float altitude;
    float spreadRate;
};

/**
 * @brief Volcanic eruption disaster handler
 *
 * Simulates a volcanic eruption with:
 * - Lava flows that follow terrain contours
 * - Pyroclastic flows (fast-moving deadly clouds)
 * - Ash clouds affecting visibility and climate
 * - Heat damage to nearby creatures
 * - Vegetation destruction
 *
 * Phases:
 * 1. Initial Eruption (0-15%): Explosion, initial lava
 * 2. Active Eruption (15-60%): Continuous lava, pyroclastic flows
 * 3. Waning Phase (60-85%): Decreasing activity
 * 4. Cooling Phase (85-100%): Lava solidifies, ash settles
 */
class VolcanoDisaster {
public:
    VolcanoDisaster();
    ~VolcanoDisaster() = default;

    /**
     * @brief Trigger a new volcanic eruption
     * @param position Eruption epicenter
     * @param radius Affected area radius
     * @param severity How severe the eruption is
     */
    void trigger(const glm::vec3& position, float radius, DisasterSeverity severity);

    /**
     * @brief Update the eruption simulation
     * @param deltaTime Time since last frame
     * @param creatures Creature manager for damage/kills
     * @param vegetation Vegetation manager for destruction
     * @param disaster Active disaster data to update
     */
    void update(float deltaTime, CreatureManager& creatures,
                VegetationManager& vegetation, ActiveDisaster& disaster);

    /**
     * @brief Reset the volcano to inactive state
     */
    void reset();

    // === Accessors ===
    bool isActive() const { return m_active; }
    const glm::vec3& getPosition() const { return m_position; }
    float getRadius() const { return m_radius; }
    float getProgress() const { return m_progress; }

    // === Visual Data for Rendering ===
    const std::vector<LavaParticle>& getLavaParticles() const { return m_lavaParticles; }
    const std::vector<LavaFlow>& getLavaFlows() const { return m_lavaFlows; }
    const std::vector<PyroclasticFlow>& getPyroclasticFlows() const { return m_pyroclasticFlows; }
    const AshCloud& getAshCloud() const { return m_ashCloud; }

    /**
     * @brief Get current eruption intensity (0-1)
     */
    float getEruptionIntensity() const;

    /**
     * @brief Get temperature at a world position
     */
    float getTemperatureAt(const glm::vec3& position) const;

    /**
     * @brief Check if position is in danger zone
     */
    bool isInDangerZone(const glm::vec3& position) const;

private:
    // === Internal Update Methods ===
    void updateLavaFlows(float deltaTime);
    void updateLavaParticles(float deltaTime);
    void updatePyroclasticFlows(float deltaTime);
    void updateAshCloud(float deltaTime);

    void spawnLavaParticles(int count);
    void spawnLavaFlow();
    void spawnPyroclasticFlow();

    void applyCreatureDamage(CreatureManager& creatures, float deltaTime, ActiveDisaster& disaster);
    void destroyVegetation(VegetationManager& vegetation, ActiveDisaster& disaster);

    float calculateHeatDamage(const glm::vec3& creaturePos, float deltaTime) const;
    glm::vec3 calculateFlowDirection(const glm::vec3& position) const;

    // === State ===
    bool m_active = false;
    glm::vec3 m_position;
    float m_radius = 50.0f;
    float m_progress = 0.0f;
    DisasterSeverity m_severity = DisasterSeverity::MODERATE;

    // === Eruption Parameters ===
    float m_eruptionIntensity = 0.0f;
    float m_lavaTemperature = 1100.0f;  // Degrees C
    float m_baseHeatDamage = 5.0f;      // Per second at epicenter
    float m_maxLavaFlows = 8;
    float m_maxPyroclasticFlows = 3;

    // === Lava System ===
    std::vector<LavaParticle> m_lavaParticles;
    std::vector<LavaFlow> m_lavaFlows;
    std::vector<PyroclasticFlow> m_pyroclasticFlows;
    AshCloud m_ashCloud;

    // === Timing ===
    float m_lavaSpawnTimer = 0.0f;
    float m_pyroclasticSpawnTimer = 0.0f;
    float m_particleSpawnAccumulator = 0.0f;

    // === Constants ===
    static constexpr int MAX_LAVA_PARTICLES = 2000;
    static constexpr float LAVA_FLOW_SPEED = 2.0f;       // Units per second
    static constexpr float PYROCLASTIC_SPEED = 15.0f;    // Much faster!
    static constexpr float ASH_SPREAD_RATE = 5.0f;

    // === Random Generation ===
    std::mt19937 m_rng;
};

} // namespace disasters
