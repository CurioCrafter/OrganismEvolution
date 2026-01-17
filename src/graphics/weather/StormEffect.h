// StormEffect.h - Storm and lightning effect system
// Handles thunderstorms, lightning flashes, and storm atmosphere
#pragma once

#include <glm/glm.hpp>
#include <random>
#include <functional>

// Forward declarations
class WeatherSystem;
class DayNightCycle;

// Lightning strike data
struct LightningStrike {
    glm::vec3 position;
    float intensity;
    float timeRemaining;
    bool isActive;
};

// Storm effect callbacks
using LightningCallback = std::function<void(const glm::vec3& position, float intensity)>;
using ThunderCallback = std::function<void(const glm::vec3& position, float distance)>;

class StormEffect {
public:
    StormEffect();
    ~StormEffect() = default;

    // Update storm effects based on weather state
    void update(float deltaTime, const WeatherSystem& weather, const DayNightCycle& dayNight);

    // Get current lightning flash intensity (0-1) for rendering
    float getLightningIntensity() const { return m_lightningIntensity; }

    // Get position of most recent lightning strike
    glm::vec3 getLightningPosition() const { return m_lightningPosition; }

    // Check if currently storming
    bool isStorming() const { return m_isStorming; }

    // Get storm darkness factor for global lighting (0 = no darkening, 1 = full storm darkness)
    float getStormDarkening() const { return m_stormDarkening; }

    // Configuration
    void setLightningInterval(float minSeconds, float maxSeconds);
    void setLightningRadius(float radius) { m_lightningRadius = radius; }
    void setStormDarkeningFactor(float factor) { m_maxStormDarkening = factor; }

    // Callbacks for audio integration
    void setLightningCallback(LightningCallback callback) { m_onLightning = callback; }
    void setThunderCallback(ThunderCallback callback) { m_onThunder = callback; }

    // Manual lightning trigger (for testing)
    void triggerLightning(const glm::vec3& position = glm::vec3(0.0f));

private:
    // Calculate next lightning strike time
    void scheduleNextLightning();

    // Update active lightning flash
    void updateLightningFlash(float deltaTime);

    // Schedule thunder delay based on distance
    void scheduleThunder(const glm::vec3& position, const glm::vec3& cameraPos);

    // Storm state
    bool m_isStorming = false;
    float m_stormDarkening = 0.0f;
    float m_targetStormDarkening = 0.0f;

    // Lightning state
    float m_lightningIntensity = 0.0f;
    glm::vec3 m_lightningPosition = glm::vec3(0.0f);
    float m_lightningTimer = 5.0f;
    float m_lightningFlashDuration = 0.2f;
    float m_lightningFlashTimer = 0.0f;

    // Lightning configuration
    float m_minLightningInterval = 3.0f;
    float m_maxLightningInterval = 15.0f;
    float m_lightningRadius = 100.0f;
    float m_maxStormDarkening = 0.3f;

    // Thunder scheduling
    struct PendingThunder {
        glm::vec3 position;
        float timeRemaining;
    };
    std::vector<PendingThunder> m_pendingThunder;

    // Callbacks
    LightningCallback m_onLightning;
    ThunderCallback m_onThunder;

    // Random generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_uniformDist;

    // Camera position (for thunder distance calculation)
    glm::vec3 m_cameraPos = glm::vec3(0.0f);

    // Transition speed
    float m_transitionSpeed = 0.5f;

    // Helper
    float randomRange(float min, float max);
};
