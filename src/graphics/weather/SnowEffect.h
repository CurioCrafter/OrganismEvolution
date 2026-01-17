// SnowEffect.h - Snow precipitation effect system
// Spawns snow particles based on weather intensity
#pragma once

#include <glm/glm.hpp>
#include <random>

// Forward declarations
class WeatherSystem;
class Camera;
class GPUParticleSystem;

class SnowEffect {
public:
    SnowEffect();
    ~SnowEffect() = default;

    // Update snow effect - spawns particles based on weather state
    void update(float deltaTime,
                const WeatherSystem& weather,
                const Camera& camera,
                GPUParticleSystem& particles);

    // Configuration
    void setSpawnRadius(float radius) { m_spawnRadius = radius; }
    void setSpawnHeight(float height) { m_spawnHeight = height; }
    void setParticlesPerSecond(float pps) { m_particlesPerSecond = pps; }
    void setFallSpeed(float speed) { m_baseFallSpeed = speed; }

    // Visual settings
    void setSnowColor(const glm::vec3& color) { m_snowColor = color; }
    void setFlakeSize(float minSize, float maxSize);

    // Snow accumulation (for terrain rendering)
    float getAccumulationAmount() const { return m_accumulationAmount; }
    void setAccumulationRate(float rate) { m_accumulationRate = rate; }
    void setMeltRate(float rate) { m_meltRate = rate; }

    // Statistics
    int getActiveParticleCount() const { return m_activeParticles; }

private:
    // Spawn parameters
    float m_spawnRadius = 40.0f;            // Radius around camera to spawn
    float m_spawnHeight = 40.0f;            // Height above camera to spawn
    float m_particlesPerSecond = 300.0f;    // Max particles per second at full intensity
    float m_baseFallSpeed = 2.0f;           // Base fall speed (slow for snow)

    // Visual parameters
    glm::vec3 m_snowColor = glm::vec3(1.0f, 1.0f, 1.0f);
    float m_minFlakeSize = 0.01f;
    float m_maxFlakeSize = 0.03f;
    float m_flakeAlpha = 0.8f;

    // Snow accumulation
    float m_accumulationAmount = 0.0f;      // 0-1, how much snow has accumulated
    float m_accumulationRate = 0.001f;      // Per second at full intensity
    float m_meltRate = 0.0001f;             // Per second when not snowing

    // State
    float m_spawnAccumulator = 0.0f;
    int m_activeParticles = 0;

    // Random generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_uniformDist;

    // Helper to get random value in range
    float randomRange(float min, float max);
};
