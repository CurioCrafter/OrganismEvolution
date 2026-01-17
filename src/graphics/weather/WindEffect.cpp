// WindEffect.cpp - Wind effect system for vegetation animation
#include "WindEffect.h"
#include "../../environment/WeatherSystem.h"
#include <algorithm>
#include <cmath>

WindEffect::WindEffect() = default;

void WindEffect::update(float deltaTime, const WeatherSystem& weather) {
    const WeatherState& state = weather.getCurrentWeather();

    // Get wind from weather system
    m_targetWindDir = glm::vec3(state.windDirection.x, 0.0f, state.windDirection.y);
    if (glm::length(m_targetWindDir) > 0.001f) {
        m_targetWindDir = glm::normalize(m_targetWindDir);
    } else {
        m_targetWindDir = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    m_targetStrength = state.windStrength;

    // Update time
    m_windTime += deltaTime;

    // Smooth transition for wind direction
    float transitionRate = m_transitionSpeed * deltaTime;

    glm::vec3 dirDiff = m_targetWindDir - m_currentWindDir;
    if (glm::length(dirDiff) > 0.001f) {
        m_currentWindDir += dirDiff * std::min(1.0f, transitionRate);
        m_currentWindDir = glm::normalize(m_currentWindDir);
    }

    // Smooth transition for wind strength
    if (std::abs(m_currentStrength - m_targetStrength) > 0.001f) {
        if (m_currentStrength < m_targetStrength) {
            m_currentStrength = std::min(m_currentStrength + transitionRate * 0.5f, m_targetStrength);
        } else {
            m_currentStrength = std::max(m_currentStrength - transitionRate * 0.5f, m_targetStrength);
        }
    }

    // Calculate gust multiplier
    m_gustMultiplier = calculateGustMultiplier(m_windTime);

    // Calculate turbulence based on storm intensity
    if (weather.isStormy()) {
        m_turbulence = m_currentStrength * 0.5f;
    } else {
        m_turbulence = m_currentStrength * 0.1f;
    }
}

WindParams WindEffect::getShaderParams() const {
    WindParams params;
    params.windDirection = m_currentWindDir;
    params.windStrength = m_currentStrength;
    params.gustMultiplier = m_gustMultiplier;
    params.windTime = m_windTime;
    params.turbulence = m_turbulence;
    params.oscillationFreq = 1.0f + m_currentStrength * 0.5f;  // Faster sway in strong wind
    return params;
}

glm::vec3 WindEffect::getWindVelocityAt(const glm::vec3& position) const {
    // Base wind velocity
    glm::vec3 baseWind = m_currentWindDir * m_currentStrength * m_gustMultiplier;

    // Add local turbulence
    float turb = calculateTurbulence(position);
    glm::vec3 turbulenceOffset(
        std::sin(position.x * 0.1f + m_windTime) * turb,
        0.0f,
        std::cos(position.z * 0.1f + m_windTime * 1.3f) * turb
    );

    return baseWind + turbulenceOffset * m_turbulence;
}

float WindEffect::calculateGustMultiplier(float time) {
    // Combine multiple frequencies for natural-feeling gusts
    // Primary gust wave
    float gust1 = std::sin(time * m_gustFrequency) * m_gustAmplitude;

    // Secondary faster variation
    float gust2 = std::sin(time * m_gustFrequency * 2.6f) * m_gustAmplitude * 0.5f;

    // Tertiary slow variation
    float gust3 = std::sin(time * m_gustFrequency * 0.3f) * m_gustAmplitude * 0.3f;

    // Random-ish variation
    float gust4 = std::sin(time * 1.7f + 3.14f) * std::sin(time * 0.7f) * m_gustAmplitude * 0.2f;

    return 1.0f + gust1 + gust2 + gust3 + gust4;
}

float WindEffect::calculateTurbulence(const glm::vec3& position) const {
    // Simple 3D noise approximation using sine functions
    float noise = std::sin(position.x * m_turbulenceScale + m_windTime) *
                  std::cos(position.z * m_turbulenceScale * 1.3f + m_windTime * 0.7f) *
                  std::sin(position.y * m_turbulenceScale * 0.5f + m_windTime * 0.5f);

    return (noise + 1.0f) * 0.5f * m_currentStrength;
}
