// StormEffect.cpp - Storm and lightning effect system
#include "StormEffect.h"
#include "../../environment/WeatherSystem.h"
#include "../../core/DayNightCycle.h"
#include <algorithm>
#include <cmath>

// Speed of sound for thunder delay calculation (approx 343 m/s)
static constexpr float SPEED_OF_SOUND = 343.0f;

StormEffect::StormEffect()
    : m_rng(std::random_device{}())
    , m_uniformDist(0.0f, 1.0f)
{
    scheduleNextLightning();
}

void StormEffect::update(float deltaTime, const WeatherSystem& weather, const DayNightCycle& dayNight) {
    // Check if we're in a storm
    bool wasStorming = m_isStorming;
    m_isStorming = weather.isStormy() &&
                   (weather.getWeatherType() == WeatherType::THUNDERSTORM);

    // Update storm darkening
    if (m_isStorming) {
        m_targetStormDarkening = m_maxStormDarkening;
    } else {
        m_targetStormDarkening = 0.0f;
    }

    // Smooth transition for storm darkening
    float transitionRate = m_transitionSpeed * deltaTime;
    if (std::abs(m_stormDarkening - m_targetStormDarkening) > 0.001f) {
        if (m_stormDarkening < m_targetStormDarkening) {
            m_stormDarkening = std::min(m_stormDarkening + transitionRate, m_targetStormDarkening);
        } else {
            m_stormDarkening = std::max(m_stormDarkening - transitionRate, m_targetStormDarkening);
        }
    }

    // Update lightning if storming
    if (m_isStorming) {
        m_lightningTimer -= deltaTime;

        if (m_lightningTimer <= 0.0f) {
            triggerLightning();
            scheduleNextLightning();
        }

        // Update active lightning flash
        updateLightningFlash(deltaTime);
    } else {
        // Decay any remaining lightning
        if (m_lightningIntensity > 0.0f) {
            m_lightningIntensity -= deltaTime * 5.0f;
            m_lightningIntensity = std::max(0.0f, m_lightningIntensity);
        }

        // Reset timer if storm just ended
        if (wasStorming) {
            scheduleNextLightning();
        }
    }

    // Update pending thunder
    for (auto it = m_pendingThunder.begin(); it != m_pendingThunder.end(); ) {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f) {
            // Thunder plays now
            if (m_onThunder) {
                float distance = glm::length(it->position - m_cameraPos);
                m_onThunder(it->position, distance);
            }
            it = m_pendingThunder.erase(it);
        } else {
            ++it;
        }
    }
}

void StormEffect::triggerLightning(const glm::vec3& position) {
    // If position not specified, generate random position around camera
    if (position == glm::vec3(0.0f)) {
        float angle = randomRange(0.0f, 6.28318f);
        float distance = randomRange(20.0f, m_lightningRadius);
        float height = randomRange(50.0f, 150.0f);

        m_lightningPosition = glm::vec3(
            m_cameraPos.x + std::cos(angle) * distance,
            m_cameraPos.y + height,
            m_cameraPos.z + std::sin(angle) * distance
        );
    } else {
        m_lightningPosition = position;
    }

    // Start flash
    m_lightningIntensity = 1.0f;
    m_lightningFlashTimer = m_lightningFlashDuration;

    // Callback for audio/visual effects
    if (m_onLightning) {
        m_onLightning(m_lightningPosition, m_lightningIntensity);
    }

    // Schedule thunder
    scheduleThunder(m_lightningPosition, m_cameraPos);
}

void StormEffect::setLightningInterval(float minSeconds, float maxSeconds) {
    m_minLightningInterval = minSeconds;
    m_maxLightningInterval = maxSeconds;
}

void StormEffect::scheduleNextLightning() {
    m_lightningTimer = randomRange(m_minLightningInterval, m_maxLightningInterval);
}

void StormEffect::updateLightningFlash(float deltaTime) {
    if (m_lightningFlashTimer > 0.0f) {
        m_lightningFlashTimer -= deltaTime;

        // Multi-flash pattern for realistic lightning
        float normalizedTime = 1.0f - (m_lightningFlashTimer / m_lightningFlashDuration);

        // Create multiple quick flashes
        if (normalizedTime < 0.1f) {
            m_lightningIntensity = 1.0f;
        } else if (normalizedTime < 0.2f) {
            m_lightningIntensity = 0.3f;
        } else if (normalizedTime < 0.3f) {
            m_lightningIntensity = 0.8f;
        } else if (normalizedTime < 0.4f) {
            m_lightningIntensity = 0.2f;
        } else if (normalizedTime < 0.5f) {
            m_lightningIntensity = 0.5f;
        } else {
            // Gradual fade
            m_lightningIntensity = 0.5f * (1.0f - (normalizedTime - 0.5f) * 2.0f);
        }
    } else {
        // Natural decay
        m_lightningIntensity -= deltaTime * 3.0f;
        m_lightningIntensity = std::max(0.0f, m_lightningIntensity);
    }
}

void StormEffect::scheduleThunder(const glm::vec3& position, const glm::vec3& cameraPos) {
    float distance = glm::length(position - cameraPos);

    // Thunder delay based on distance and speed of sound
    // Scale down for gameplay (real world would be too slow)
    float delay = (distance / SPEED_OF_SOUND) * 0.3f;  // 30% of real delay
    delay = std::max(0.1f, std::min(delay, 5.0f));     // Clamp to reasonable range

    PendingThunder thunder;
    thunder.position = position;
    thunder.timeRemaining = delay;
    m_pendingThunder.push_back(thunder);
}

float StormEffect::randomRange(float min, float max) {
    return min + m_uniformDist(m_rng) * (max - min);
}
