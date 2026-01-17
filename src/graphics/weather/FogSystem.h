// FogSystem.h - Atmospheric fog rendering system
// Provides shader parameters for distance fog, height fog, and weather-based fog
#pragma once

#include <glm/glm.hpp>

// Forward declarations
class WeatherSystem;
class DayNightCycle;

// Fog parameters for GPU shaders
struct FogParams {
    glm::vec3 fogColor = glm::vec3(0.8f, 0.85f, 0.9f);
    float fogDensity = 0.0f;
    float fogStart = 10.0f;
    float fogEnd = 200.0f;
    float heightFogDensity = 0.0f;
    float heightFogFalloff = 0.02f;
    float heightFogBase = 0.0f;      // Base height for height fog
    float scatteringCoeff = 0.02f;   // For volumetric fog
    float absorptionCoeff = 0.01f;   // For volumetric fog
    float mieG = 0.75f;              // Mie scattering asymmetry
};

class FogSystem {
public:
    FogSystem();
    ~FogSystem() = default;

    // Update fog based on weather and time of day
    void update(float deltaTime, const WeatherSystem& weather, const DayNightCycle& dayNight);

    // Get shader-ready fog parameters
    FogParams getShaderParams() const;

    // Manual overrides for specific scenarios
    void setOverrideFogDensity(float density);
    void setOverrideFogColor(const glm::vec3& color);
    void clearOverrides();

    // Configuration
    void setFogStartDistance(float start) { m_fogStart = start; }
    void setFogEndDistance(float end) { m_fogEnd = end; }
    void setHeightFogParameters(float density, float falloff, float baseHeight);
    void setVolumetricParameters(float scattering, float absorption, float mieG);

    // Accessors
    float getCurrentDensity() const { return m_currentDensity; }
    glm::vec3 getCurrentColor() const { return m_currentColor; }
    bool isVolumetricEnabled() const { return m_enableVolumetric; }
    void setVolumetricEnabled(bool enabled) { m_enableVolumetric = enabled; }

private:
    // Calculate fog color based on sky colors
    glm::vec3 calculateFogColor(const DayNightCycle& dayNight);

    // Morning fog burn-off calculation
    float calculateMorningBurnOff(float timeOfDay, float baseDensity);

    // Current state (smoothly interpolated)
    float m_currentDensity = 0.0f;
    glm::vec3 m_currentColor = glm::vec3(0.8f);
    float m_targetDensity = 0.0f;
    glm::vec3 m_targetColor = glm::vec3(0.8f);

    // Distance fog parameters
    float m_fogStart = 10.0f;
    float m_fogEnd = 200.0f;

    // Height fog parameters
    float m_heightFogDensity = 0.0f;
    float m_heightFogFalloff = 0.02f;
    float m_heightFogBase = 0.0f;

    // Volumetric fog parameters
    float m_scatteringCoeff = 0.02f;
    float m_absorptionCoeff = 0.01f;
    float m_mieG = 0.75f;
    bool m_enableVolumetric = true;

    // Override values
    bool m_hasOverrideDensity = false;
    bool m_hasOverrideColor = false;
    float m_overrideDensity = 0.0f;
    glm::vec3 m_overrideColor = glm::vec3(0.8f);

    // Transition speed
    float m_transitionSpeed = 0.5f;  // Units per second
};
