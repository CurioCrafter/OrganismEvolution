// WindEffect.h - Wind effect system for vegetation animation
// Provides wind parameters for grass, trees, and other vegetation bending
#pragma once

#include <glm/glm.hpp>

// Forward declarations
class WeatherSystem;

// Wind parameters for GPU shaders
struct WindParams {
    glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    float windStrength = 0.0f;
    float gustMultiplier = 1.0f;
    float windTime = 0.0f;
    float turbulence = 0.0f;        // Higher during storms
    float oscillationFreq = 1.0f;   // Grass sway frequency
};

class WindEffect {
public:
    WindEffect();
    ~WindEffect() = default;

    // Update wind based on weather state
    void update(float deltaTime, const WeatherSystem& weather);

    // Get shader-ready wind parameters
    WindParams getShaderParams() const;

    // Get wind velocity at a specific position (for particle physics)
    glm::vec3 getWindVelocityAt(const glm::vec3& position) const;

    // Configuration
    void setGustFrequency(float freq) { m_gustFrequency = freq; }
    void setGustAmplitude(float amp) { m_gustAmplitude = amp; }
    void setTurbulenceScale(float scale) { m_turbulenceScale = scale; }

    // Direct access to current wind state
    glm::vec3 getWindDirection() const { return m_currentWindDir; }
    float getWindStrength() const { return m_currentStrength; }
    float getGustMultiplier() const { return m_gustMultiplier; }

private:
    // Calculate gust variations
    float calculateGustMultiplier(float time);

    // Calculate local turbulence at position
    float calculateTurbulence(const glm::vec3& position) const;

    // Current state
    glm::vec3 m_currentWindDir = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_targetWindDir = glm::vec3(1.0f, 0.0f, 0.0f);
    float m_currentStrength = 0.0f;
    float m_targetStrength = 0.0f;
    float m_gustMultiplier = 1.0f;
    float m_turbulence = 0.0f;

    // Time tracking
    float m_windTime = 0.0f;

    // Gust parameters
    float m_gustFrequency = 0.5f;
    float m_gustAmplitude = 0.3f;
    float m_gustPhase = 0.0f;

    // Turbulence parameters
    float m_turbulenceScale = 0.1f;

    // Transition speed
    float m_transitionSpeed = 2.0f;
};
