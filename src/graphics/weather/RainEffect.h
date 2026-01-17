// RainEffect.h - Rain precipitation effect system
// Spawns rain particles based on weather intensity
#pragma once

#include <glm/glm.hpp>
#include <random>

// Forward declarations
class WeatherSystem;
class Terrain;
class Camera;
class GPUParticleSystem;

class RainEffect {
public:
    RainEffect();
    ~RainEffect() = default;

    // Update rain effect - spawns particles based on weather state
    void update(float deltaTime,
                const WeatherSystem& weather,
                const Terrain& terrain,
                const Camera& camera,
                GPUParticleSystem& particles);

    // Configuration
    void setSpawnRadius(float radius) { m_spawnRadius = radius; }
    void setSpawnHeight(float height) { m_spawnHeight = height; }
    void setParticlesPerSecond(float pps) { m_particlesPerSecond = pps; }
    void setDropSpeed(float speed) { m_baseDropSpeed = speed; }

    // Visual settings
    void setRainColor(const glm::vec3& color) { m_rainColor = color; }
    void setDropSize(float size) { m_dropSize = size; }

    // Statistics
    int getActiveParticleCount() const { return m_activeParticles; }

private:
    // Spawn parameters
    float m_spawnRadius = 50.0f;            // Radius around camera to spawn
    float m_spawnHeight = 50.0f;            // Height above camera to spawn
    float m_particlesPerSecond = 500.0f;    // Max particles per second at full intensity
    float m_baseDropSpeed = 15.0f;          // Base fall speed

    // Visual parameters
    glm::vec3 m_rainColor = glm::vec3(0.7f, 0.75f, 0.85f);
    float m_dropSize = 0.02f;
    float m_dropAlpha = 0.3f;

    // State
    float m_spawnAccumulator = 0.0f;
    int m_activeParticles = 0;

    // Random generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_uniformDist;

    // Helper to get random value in range
    float randomRange(float min, float max);
};
