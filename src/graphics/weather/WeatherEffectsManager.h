// WeatherEffectsManager.h - Central coordinator for all weather visual effects
// Orchestrates rain, snow, fog, wind, and storm effects
#pragma once

#include "RainEffect.h"
#include "SnowEffect.h"
#include "FogSystem.h"
#include "WindEffect.h"
#include "StormEffect.h"
#include "../particles/GPUParticleSystem.h"
#include <memory>
#include <functional>

// Forward declarations
class DX12Device;
class WeatherSystem;
class ClimateSystem;
class BiomeSystem;
class DayNightCycle;
class Terrain;
class Camera;

// Callback types for audio integration
using AudioLightningCallback = std::function<void(const glm::vec3& position, float intensity)>;
using AudioThunderCallback = std::function<void(const glm::vec3& position, float distance)>;
using AudioRainCallback = std::function<void(float intensity)>;
using AudioWindCallback = std::function<void(float strength, const glm::vec3& direction)>;

// Weather rendering parameters for main render loop
struct WeatherRenderParams {
    FogParams fog;
    WindParams wind;
    float stormDarkening;
    float lightningIntensity;
    glm::vec3 lightningPosition;
    float snowAccumulation;
    float groundWetness;
};

class WeatherEffectsManager {
public:
    WeatherEffectsManager();
    ~WeatherEffectsManager();

    // Non-copyable
    WeatherEffectsManager(const WeatherEffectsManager&) = delete;
    WeatherEffectsManager& operator=(const WeatherEffectsManager&) = delete;

    // Initialize all weather systems
    bool initialize(DX12Device* device);
    void shutdown();

    // Set references to external systems (must be called after initialize)
    void setWeatherSystem(WeatherSystem* weather) { m_weather = weather; }
    void setClimateSystem(ClimateSystem* climate) { m_climate = climate; }
    void setBiomeSystem(BiomeSystem* biomes) { m_biomes = biomes; }
    void setDayNightCycle(DayNightCycle* dayNight) { m_dayNight = dayNight; }
    void setTerrain(Terrain* terrain);

    // Update all weather effects (call each frame)
    void update(float deltaTime, const Camera& camera);

    // Render weather particles (call during render pass)
    void render(ID3D12GraphicsCommandList* cmdList,
                const Camera& camera,
                const glm::mat4& viewProjection,
                float time);

    // Get render parameters for other systems to use
    WeatherRenderParams getRenderParams() const;

    // Audio callbacks
    void setLightningCallback(AudioLightningCallback callback);
    void setThunderCallback(AudioThunderCallback callback);
    void setRainIntensityCallback(AudioRainCallback callback);
    void setWindCallback(AudioWindCallback callback);

    // Access individual systems for advanced configuration
    RainEffect& getRainEffect() { return m_rainEffect; }
    SnowEffect& getSnowEffect() { return m_snowEffect; }
    FogSystem& getFogSystem() { return m_fogSystem; }
    WindEffect& getWindEffect() { return m_windEffect; }
    StormEffect& getStormEffect() { return m_stormEffect; }
    GPUParticleSystem& getParticleSystem() { return m_particleSystem; }

    const RainEffect& getRainEffect() const { return m_rainEffect; }
    const SnowEffect& getSnowEffect() const { return m_snowEffect; }
    const FogSystem& getFogSystem() const { return m_fogSystem; }
    const WindEffect& getWindEffect() const { return m_windEffect; }
    const StormEffect& getStormEffect() const { return m_stormEffect; }

    // Statistics
    int getTotalParticleCount() const;
    bool isInitialized() const { return m_initialized; }

    // Enable/disable individual effects
    void setRainEnabled(bool enabled) { m_rainEnabled = enabled; }
    void setSnowEnabled(bool enabled) { m_snowEnabled = enabled; }
    void setFogEnabled(bool enabled) { m_fogEnabled = enabled; }
    void setStormEnabled(bool enabled) { m_stormEnabled = enabled; }

    bool isRainEnabled() const { return m_rainEnabled; }
    bool isSnowEnabled() const { return m_snowEnabled; }
    bool isFogEnabled() const { return m_fogEnabled; }
    bool isStormEnabled() const { return m_stormEnabled; }

private:
    // Update audio callbacks
    void updateAudioCallbacks();

    // External system references
    WeatherSystem* m_weather = nullptr;
    ClimateSystem* m_climate = nullptr;
    BiomeSystem* m_biomes = nullptr;
    DayNightCycle* m_dayNight = nullptr;
    Terrain* m_terrain = nullptr;

    // Weather effect systems
    GPUParticleSystem m_particleSystem;
    RainEffect m_rainEffect;
    SnowEffect m_snowEffect;
    FogSystem m_fogSystem;
    WindEffect m_windEffect;
    StormEffect m_stormEffect;

    // Audio callbacks
    AudioLightningCallback m_onLightning;
    AudioThunderCallback m_onThunder;
    AudioRainCallback m_onRainIntensity;
    AudioWindCallback m_onWind;

    // Cached values for audio
    float m_lastRainIntensity = 0.0f;
    float m_lastWindStrength = 0.0f;

    // Enable flags
    bool m_rainEnabled = true;
    bool m_snowEnabled = true;
    bool m_fogEnabled = true;
    bool m_stormEnabled = true;

    // Ground wetness (builds up during rain, dries over time)
    float m_groundWetness = 0.0f;

    bool m_initialized = false;
};
