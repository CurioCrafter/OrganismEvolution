// WeatherEffectsManager.cpp - Central coordinator for all weather visual effects
#include "WeatherEffectsManager.h"
#include "../DX12Device.h"
#include "../Camera.h"
#include "../../environment/WeatherSystem.h"
#include "../../environment/ClimateSystem.h"
#include "../../environment/BiomeSystem.h"
#include "../../environment/Terrain.h"
#include "../../core/DayNightCycle.h"
#include <algorithm>

WeatherEffectsManager::WeatherEffectsManager() = default;

WeatherEffectsManager::~WeatherEffectsManager() {
    shutdown();
}

bool WeatherEffectsManager::initialize(DX12Device* device) {
    if (m_initialized) {
        return true;
    }

    if (!device) {
        return false;
    }

    // Initialize GPU particle system
    if (!m_particleSystem.initialize(device)) {
        return false;
    }

    // Set up storm effect callbacks to route through our callbacks
    m_stormEffect.setLightningCallback([this](const glm::vec3& pos, float intensity) {
        if (m_onLightning) {
            m_onLightning(pos, intensity);
        }
    });

    m_stormEffect.setThunderCallback([this](const glm::vec3& pos, float distance) {
        if (m_onThunder) {
            m_onThunder(pos, distance);
        }
    });

    m_initialized = true;
    return true;
}

void WeatherEffectsManager::shutdown() {
    m_particleSystem.shutdown();
    m_initialized = false;
}

void WeatherEffectsManager::setTerrain(Terrain* terrain) {
    m_terrain = terrain;
    m_particleSystem.setTerrain(terrain);
}

void WeatherEffectsManager::update(float deltaTime, const Camera& camera) {
    if (!m_initialized || !m_weather || !m_dayNight) {
        return;
    }

    const WeatherState& weatherState = m_weather->getCurrentWeather();

    // Update wind effect (always runs, provides data for vegetation)
    m_windEffect.update(deltaTime, *m_weather);

    // Update fog system
    if (m_fogEnabled) {
        m_fogSystem.update(deltaTime, *m_weather, *m_dayNight);
    }

    // Update storm effect (handles lightning timing)
    if (m_stormEnabled) {
        m_stormEffect.update(deltaTime, *m_weather, *m_dayNight);
    }

    // Update ground wetness
    if (m_weather->isRaining()) {
        float wetRate = weatherState.precipitationIntensity * 0.1f;
        m_groundWetness += wetRate * deltaTime;
        m_groundWetness = std::min(m_groundWetness, 1.0f);
    } else {
        float dryRate = 0.02f;
        m_groundWetness -= dryRate * deltaTime;
        m_groundWetness = std::max(m_groundWetness, 0.0f);
    }

    // Get ground level for particle collision
    float groundLevel = 0.0f;
    if (m_terrain) {
        groundLevel = m_terrain->getHeight(camera.Position.x, camera.Position.z);
    }

    // Update particle system with weather state
    WindParams wind = m_windEffect.getShaderParams();
    m_particleSystem.update(
        deltaTime,
        wind.windDirection,
        wind.windStrength,
        weatherState.precipitationIntensity,
        weatherState.precipitationType,
        weatherState.fogDensity,
        groundLevel,
        camera.Position
    );

    // Spawn rain particles
    if (m_rainEnabled && m_terrain) {
        m_rainEffect.update(deltaTime, *m_weather, *m_terrain, camera, m_particleSystem);
    }

    // Spawn snow particles
    if (m_snowEnabled) {
        m_snowEffect.update(deltaTime, *m_weather, camera, m_particleSystem);
    }

    // Update audio callbacks
    updateAudioCallbacks();
}

void WeatherEffectsManager::render(ID3D12GraphicsCommandList* cmdList,
                                    const Camera& camera,
                                    const glm::mat4& viewProjection,
                                    float time) {
    if (!m_initialized) {
        return;
    }

    // Render weather particles
    float lightningIntensity = m_stormEnabled ? m_stormEffect.getLightningIntensity() : 0.0f;
    glm::vec3 lightningPos = m_stormEnabled ? m_stormEffect.getLightningPosition() : glm::vec3(0.0f);

    m_particleSystem.render(cmdList, camera, viewProjection, time, lightningIntensity, lightningPos);
}

WeatherRenderParams WeatherEffectsManager::getRenderParams() const {
    WeatherRenderParams params;

    params.fog = m_fogSystem.getShaderParams();
    params.wind = m_windEffect.getShaderParams();
    params.stormDarkening = m_stormEnabled ? m_stormEffect.getStormDarkening() : 0.0f;
    params.lightningIntensity = m_stormEnabled ? m_stormEffect.getLightningIntensity() : 0.0f;
    params.lightningPosition = m_stormEnabled ? m_stormEffect.getLightningPosition() : glm::vec3(0.0f);
    params.snowAccumulation = m_snowEffect.getAccumulationAmount();
    params.groundWetness = m_groundWetness;

    return params;
}

void WeatherEffectsManager::setLightningCallback(AudioLightningCallback callback) {
    m_onLightning = callback;
}

void WeatherEffectsManager::setThunderCallback(AudioThunderCallback callback) {
    m_onThunder = callback;
}

void WeatherEffectsManager::setRainIntensityCallback(AudioRainCallback callback) {
    m_onRainIntensity = callback;
}

void WeatherEffectsManager::setWindCallback(AudioWindCallback callback) {
    m_onWind = callback;
}

int WeatherEffectsManager::getTotalParticleCount() const {
    return m_rainEffect.getActiveParticleCount() + m_snowEffect.getActiveParticleCount();
}

void WeatherEffectsManager::updateAudioCallbacks() {
    if (!m_weather) {
        return;
    }

    const WeatherState& state = m_weather->getCurrentWeather();

    // Notify rain intensity changes
    if (m_onRainIntensity) {
        float rainIntensity = m_weather->isRaining() ? state.precipitationIntensity : 0.0f;
        if (std::abs(rainIntensity - m_lastRainIntensity) > 0.01f) {
            m_onRainIntensity(rainIntensity);
            m_lastRainIntensity = rainIntensity;
        }
    }

    // Notify wind changes
    if (m_onWind) {
        float windStrength = state.windStrength;
        if (std::abs(windStrength - m_lastWindStrength) > 0.01f) {
            glm::vec3 windDir = glm::vec3(state.windDirection.x, 0.0f, state.windDirection.y);
            m_onWind(windStrength, windDir);
            m_lastWindStrength = windStrength;
        }
    }
}
