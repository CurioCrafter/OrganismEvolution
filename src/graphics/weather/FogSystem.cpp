// FogSystem.cpp - Atmospheric fog rendering system
#include "FogSystem.h"
#include "../../environment/WeatherSystem.h"
#include "../../core/DayNightCycle.h"
#include <algorithm>
#include <cmath>

FogSystem::FogSystem() = default;

void FogSystem::update(float deltaTime, const WeatherSystem& weather, const DayNightCycle& dayNight) {
    const WeatherState& state = weather.getCurrentWeather();

    // Get base fog density from weather
    float baseDensity = state.fogDensity;

    // Apply morning burn-off effect
    float timeOfDay = dayNight.dayTime;
    baseDensity = calculateMorningBurnOff(timeOfDay, baseDensity);

    // Set target values
    m_targetDensity = baseDensity;
    m_targetColor = calculateFogColor(dayNight);

    // Smooth transition to target values
    float transitionRate = m_transitionSpeed * deltaTime;

    // Interpolate density
    if (std::abs(m_currentDensity - m_targetDensity) > 0.001f) {
        if (m_currentDensity < m_targetDensity) {
            m_currentDensity = std::min(m_currentDensity + transitionRate, m_targetDensity);
        } else {
            m_currentDensity = std::max(m_currentDensity - transitionRate, m_targetDensity);
        }
    }

    // Interpolate color
    glm::vec3 colorDiff = m_targetColor - m_currentColor;
    if (glm::length(colorDiff) > 0.001f) {
        m_currentColor += glm::normalize(colorDiff) * std::min(glm::length(colorDiff), transitionRate);
    }

    // Update height fog based on weather
    if (weather.isFoggy()) {
        m_heightFogDensity = baseDensity * 0.5f;
    } else {
        m_heightFogDensity = 0.0f;
    }
}

FogParams FogSystem::getShaderParams() const {
    FogParams params;

    // Apply overrides if set
    params.fogDensity = m_hasOverrideDensity ? m_overrideDensity : m_currentDensity;
    params.fogColor = m_hasOverrideColor ? m_overrideColor : m_currentColor;

    params.fogStart = m_fogStart;
    params.fogEnd = m_fogEnd;
    params.heightFogDensity = m_heightFogDensity;
    params.heightFogFalloff = m_heightFogFalloff;
    params.heightFogBase = m_heightFogBase;
    params.scatteringCoeff = m_scatteringCoeff;
    params.absorptionCoeff = m_absorptionCoeff;
    params.mieG = m_mieG;

    return params;
}

void FogSystem::setOverrideFogDensity(float density) {
    m_hasOverrideDensity = true;
    m_overrideDensity = density;
}

void FogSystem::setOverrideFogColor(const glm::vec3& color) {
    m_hasOverrideColor = true;
    m_overrideColor = color;
}

void FogSystem::clearOverrides() {
    m_hasOverrideDensity = false;
    m_hasOverrideColor = false;
}

void FogSystem::setHeightFogParameters(float density, float falloff, float baseHeight) {
    m_heightFogDensity = density;
    m_heightFogFalloff = falloff;
    m_heightFogBase = baseHeight;
}

void FogSystem::setVolumetricParameters(float scattering, float absorption, float mieG) {
    m_scatteringCoeff = scattering;
    m_absorptionCoeff = absorption;
    m_mieG = mieG;
}

glm::vec3 FogSystem::calculateFogColor(const DayNightCycle& dayNight) {
    // Get sky colors for blending
    SkyColors skyColors = dayNight.GetSkyColors();

    // Fog color is mostly horizon color with some adjustment
    glm::vec3 horizonColor = glm::vec3(skyColors.skyHorizon.x, skyColors.skyHorizon.y, skyColors.skyHorizon.z);

    // Add slight desaturation and brightening for atmospheric scattering effect
    glm::vec3 fogBase = glm::mix(horizonColor, glm::vec3(0.8f, 0.85f, 0.9f), 0.3f);

    // Adjust based on time of day
    if (dayNight.IsNight()) {
        // Night fog is darker and more blue
        fogBase = glm::mix(fogBase, glm::vec3(0.1f, 0.12f, 0.18f), 0.7f);
    } else if (dayNight.IsDawn() || dayNight.IsDusk()) {
        // Golden hour fog has warm tint
        fogBase = glm::mix(fogBase, glm::vec3(0.9f, 0.7f, 0.5f), 0.2f);
    }

    return fogBase;
}

float FogSystem::calculateMorningBurnOff(float timeOfDay, float baseDensity) {
    // Morning burn-off: fog dissipates between ~6am (0.25) and ~10am (0.42)
    // timeOfDay: 0 = midnight, 0.25 = dawn, 0.5 = noon

    if (baseDensity < 0.01f) {
        return baseDensity;
    }

    // Pre-dawn and early morning: fog at full strength
    if (timeOfDay < 0.25f) {
        return baseDensity;
    }

    // Morning burn-off period
    if (timeOfDay >= 0.25f && timeOfDay < 0.42f) {
        float burnOffProgress = (timeOfDay - 0.25f) / 0.17f;  // 0 to 1 over burn-off period
        float burnOffFactor = 1.0f - burnOffProgress * 0.7f;  // Reduce by up to 70%
        return baseDensity * burnOffFactor;
    }

    // Mid-day: fog at reduced strength
    if (timeOfDay >= 0.42f && timeOfDay < 0.7f) {
        return baseDensity * 0.3f;
    }

    // Evening: fog can build up again
    if (timeOfDay >= 0.7f && timeOfDay < 0.85f) {
        float buildUpProgress = (timeOfDay - 0.7f) / 0.15f;
        float buildUpFactor = 0.3f + buildUpProgress * 0.7f;
        return baseDensity * buildUpFactor;
    }

    // Night: full fog
    return baseDensity;
}
