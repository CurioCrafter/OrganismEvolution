// RainEffect.cpp - Rain precipitation effect system
#include "RainEffect.h"
#include "../../environment/WeatherSystem.h"
#include "../../environment/Terrain.h"
#include "../Camera.h"
#include "../particles/GPUParticleSystem.h"
#include <algorithm>
#include <cmath>

RainEffect::RainEffect()
    : m_rng(std::random_device{}())
    , m_uniformDist(0.0f, 1.0f)
{
}

void RainEffect::update(float deltaTime,
                        const WeatherSystem& weather,
                        const Terrain& terrain,
                        const Camera& camera,
                        GPUParticleSystem& particles) {
    // Get rain intensity from weather system
    const WeatherState& state = weather.getCurrentWeather();
    float rainIntensity = state.precipitationIntensity;

    // Only spawn rain particles if it's actually raining (not snow)
    if (!weather.isRaining() || rainIntensity < 0.01f) {
        m_activeParticles = 0;
        return;
    }

    // Calculate particles to spawn this frame
    float particlesToSpawnFloat = rainIntensity * m_particlesPerSecond * deltaTime;
    particlesToSpawnFloat += m_spawnAccumulator;

    int particlesToSpawn = static_cast<int>(particlesToSpawnFloat);
    m_spawnAccumulator = particlesToSpawnFloat - static_cast<float>(particlesToSpawn);

    // Hard cap per frame to prevent spikes
    particlesToSpawn = std::min(particlesToSpawn, 200);

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

        // Check terrain height to avoid spawning below ground
        float terrainHeight = terrain.getHeight(emit.position.x, emit.position.z);
        if (emit.position.y < terrainHeight + 5.0f) {
            emit.position.y = terrainHeight + m_spawnHeight;
        }

        // Velocity - falling with wind influence
        // Rain falls fast, wind has moderate effect
        emit.velocity = glm::vec3(
            windVec.x * 5.0f + randomRange(-0.5f, 0.5f),
            -m_baseDropSpeed - randomRange(0.0f, 3.0f),
            windVec.z * 5.0f + randomRange(-0.5f, 0.5f)
        );

        // Rain drop properties
        emit.life = 4.0f;  // Lifetime in seconds
        emit.size = m_dropSize + randomRange(0.0f, 0.02f);
        emit.rotation = 0.0f;  // Rain doesn't tumble
        emit.type = 0.0f;  // 0 = rain

        particles.emit(emit);
    }

    m_activeParticles = particlesToSpawn;
}

float RainEffect::randomRange(float min, float max) {
    return min + m_uniformDist(m_rng) * (max - min);
}
