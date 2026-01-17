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
class ClimateSystem;
struct ActiveDisaster;

using Forge::CreatureManager;

namespace disasters {

/**
 * @brief Phases of meteor impact
 */
enum class MeteorPhase {
    INCOMING,       // Meteor approaching (warning phase)
    IMPACT,         // Initial collision
    SHOCKWAVE,      // Expanding shockwave
    DEBRIS,         // Debris rain and fires
    DUST_CLOUD,     // Nuclear winter effect
    RECOVERY        // Slow recovery
};

/**
 * @brief Debris particle from impact
 */
struct ImpactDebris {
    glm::vec3 position;
    glm::vec3 velocity;
    float size;
    float lifetime;
    bool onFire;
    bool active;
};

/**
 * @brief Impact crater data
 */
struct ImpactCrater {
    glm::vec3 center;
    float radius;
    float depth;
    float rimHeight;
    bool formed;
};

/**
 * @brief Shockwave from impact
 */
struct Shockwave {
    glm::vec3 origin;
    float currentRadius;
    float maxRadius;
    float speed;
    float intensity;    // Decreases as it expands
    bool active;
};

/**
 * @brief Meteor impact disaster handler
 *
 * Simulates a meteor/asteroid impact with:
 * - Visible incoming meteor (warning period)
 * - Explosive impact with shockwave
 * - Crater formation (visual only - terrain not deformed)
 * - Debris rain causing secondary damage
 * - Dust cloud blocking sunlight (nuclear winter)
 * - Long-term climate cooling
 *
 * Phases:
 * 1. Incoming (0-5%): Warning, meteor visible in sky
 * 2. Impact (5-10%): Explosion, initial casualties
 * 3. Shockwave (10-25%): Expanding blast wave
 * 4. Debris (25-45%): Falling debris, fires
 * 5. Dust Cloud (45-90%): Nuclear winter, reduced sunlight
 * 6. Recovery (90-100%): Climate slowly returns to normal
 */
class MeteorImpact {
public:
    MeteorImpact();
    ~MeteorImpact() = default;

    /**
     * @brief Trigger a meteor impact
     * @param position Impact position
     * @param radius Crater/effect radius
     * @param severity Impact severity
     */
    void trigger(const glm::vec3& position, float radius, DisasterSeverity severity);

    /**
     * @brief Update the impact simulation
     */
    void update(float deltaTime, CreatureManager& creatures, Terrain& terrain,
                ClimateSystem& climate, ActiveDisaster& disaster);

    /**
     * @brief Reset to inactive state
     */
    void reset();

    // === Accessors ===
    bool isActive() const { return m_active; }
    MeteorPhase getCurrentPhase() const { return m_currentPhase; }
    const glm::vec3& getImpactPosition() const { return m_impactPosition; }
    float getCraterRadius() const { return m_crater.radius; }

    // === Visual Data ===
    glm::vec3 getMeteorPosition() const { return m_meteorPosition; }
    float getMeteorSize() const { return m_meteorSize; }
    bool isMeteorVisible() const { return m_currentPhase == MeteorPhase::INCOMING; }

    const Shockwave& getShockwave() const { return m_shockwave; }
    const ImpactCrater& getCrater() const { return m_crater; }
    const std::vector<ImpactDebris>& getDebris() const { return m_debris; }

    float getDustCloudDensity() const { return m_dustCloudDensity; }
    float getSunlightReduction() const { return m_sunlightReduction; }
    float getTemperatureOffset() const { return m_temperatureOffset; }

private:
    // === Phase Updates ===
    void updateIncomingPhase(float deltaTime, ActiveDisaster& disaster);
    void updateImpactPhase(float deltaTime, CreatureManager& creatures, ActiveDisaster& disaster);
    void updateShockwavePhase(float deltaTime, CreatureManager& creatures, ActiveDisaster& disaster);
    void updateDebrisPhase(float deltaTime, CreatureManager& creatures, ActiveDisaster& disaster);
    void updateDustCloudPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);
    void updateRecoveryPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster);

    void advancePhase(ActiveDisaster& disaster);
    void spawnDebris(int count);
    void applyShockwaveDamage(CreatureManager& creatures, float deltaTime, ActiveDisaster& disaster);
    void applyDebrisDamage(CreatureManager& creatures, float deltaTime, ActiveDisaster& disaster);
    float calculateShockwaveDamage(float distance) const;

    // === State ===
    bool m_active = false;
    MeteorPhase m_currentPhase = MeteorPhase::INCOMING;
    DisasterSeverity m_severity = DisasterSeverity::MODERATE;

    // === Meteor ===
    glm::vec3 m_meteorPosition;
    glm::vec3 m_meteorVelocity;
    float m_meteorSize = 10.0f;

    // === Impact ===
    glm::vec3 m_impactPosition;
    ImpactCrater m_crater;
    Shockwave m_shockwave;
    std::vector<ImpactDebris> m_debris;

    // === Environmental Effects ===
    float m_dustCloudDensity = 0.0f;
    float m_dustCloudRadius = 0.0f;
    float m_sunlightReduction = 0.0f;
    float m_temperatureOffset = 0.0f;

    // === Parameters ===
    float m_baseDamage = 20.0f;
    float m_shockwaveSpeed = 50.0f;
    float m_maxShockwaveRadius = 100.0f;
    float m_dustDuration = 180.0f;        // Seconds
    float m_maxCooling = -15.0f;           // Temperature reduction

    // === Timing ===
    float m_phaseTimer = 0.0f;

    // === Constants ===
    static constexpr int MAX_DEBRIS = 500;
    static constexpr float METEOR_APPROACH_SPEED = 100.0f;
    static constexpr float METEOR_START_ALTITUDE = 500.0f;

    // === Random Generation ===
    std::mt19937 m_rng;
};

} // namespace disasters
