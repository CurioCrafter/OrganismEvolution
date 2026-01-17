// SnowEffect.cpp - Snow precipitation effect system
#include "SnowEffect.h"
#include "../../environment/WeatherSystem.h"
#include "../Camera.h"
#include "../particles/GPUParticleSystem.h"
#include <algorithm>
#include <cmath>

SnowEffect::SnowEffect()
    : m_rng(std::random_device{}())
    , m_uniformDist(0.0f, 1.0f)
{
}

void SnowEffect::update(float deltaTime,
                        const WeatherSystem& weather,
                        const Camera& camera,
                        GPUParticleSystem& particles) {
    const WeatherState& state = weather.getCurrentWeather();

    // Check if snowing (precipitation type 1 = snow)
    float snowIntensity = 0.0f;
    if (weather.isSnowing()) {
        snowIntensity = state.precipitationIntensity;
    }

    // Update snow accumulation
    if (snowIntensity > 0.01f) {
        m_accumulationAmount += m_accumulationRate * snowIntensity * deltaTime;
        m_accumulationAmount = std::min(m_accumulationAmount, 1.0f);
    } else {
        // Melt when not snowing
        m_accumulationAmount -= m_meltRate * deltaTime;
        m_accumulationAmount = std::max(m_accumulationAmount, 0.0f);
    }

    // Only spawn snow particles if snowing
    if (snowIntensity < 0.01f) {
        m_activeParticles = 0;
        return;
    }

    // Calculate particles to spawn this frame
    float particlesToSpawnFloat = snowIntensity * m_particlesPerSecond * deltaTime;
    particlesToSpawnFloat += m_spawnAccumulator;

    int particlesToSpawn = static_cast<int>(particlesToSpawnFloat);
    m_spawnAccumulator = particlesToSpawnFloat - static_cast<float>(particlesToSpawn);

    // Hard cap per frame to prevent spikes
    particlesToSpawn = std::min(particlesToSpawn, 150);

    glm::vec3 camPos = camera.Position;
    glm::vec3 windVec = glm::vec3(state.windDirection.x, 0.0f, state.windDirection.y) * state.windStrength;

    for (int i = 0; i < particlesToSpawn; ++i) {
        ParticleEmitParams emit;

        // Spawn position in cylinder around camera
        float angle = randomRange(0.0f, 6.28318f);
        float radius = randomRange(0.0f, m_spawnRadius);
        float heightOffset = randomRange(0.0f, 10.0f);

        emit.position = glm::vec3(
            camPos.x + std::cos(angle) * radius,
            camPos.y + m_spawnHeight + heightOffset,
            camPos.z + std::sin(angle) * radius
        );

        // Velocity - falling slowly with wind influence
        // Snow is heavily affected by wind and drifts
        float speedVariation = randomRange(0.0f, 1.0f);
        emit.velocity = glm::vec3(
            windVec.x * 3.0f + randomRange(-1.0f, 1.0f),
            -m_baseFallSpeed - speedVariation,
            windVec.z * 3.0f + randomRange(-1.0f, 1.0f)
        );

        // Snow flake properties
        emit.life = 20.0f;  // Snow lasts longer due to slow fall
        emit.size = randomRange(m_minFlakeSize, m_maxFlakeSize);
        emit.rotation = randomRange(0.0f, 6.28318f);  // Initial rotation
        emit.type = 1.0f;  // 1 = snow

        particles.emit(emit);
    }

    m_activeParticles = particlesToSpawn;
}

void SnowEffect::setFlakeSize(float minSize, float maxSize) {
    m_minFlakeSize = minSize;
    m_maxFlakeSize = maxSize;
}

float SnowEffect::randomRange(float min, float max) {
    return min + m_uniformDist(m_rng) * (max - min);
}
