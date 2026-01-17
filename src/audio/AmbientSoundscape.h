#pragma once

// AmbientSoundscape - Biome-based ambient layers with time-of-day and weather integration
//
// Design:
// - Each biome has characteristic ambient sound layers
// - Time of day modulates which sounds play (dawn chorus, night insects)
// - Weather adds additional layers (rain, wind, thunder)
// - All layers crossfade smoothly (no jarring transitions)
// - Maximum 4 ambient layers active at once (hard limit)

#include "AudioManager.h"
#include "ProceduralSynthesizer.h"
#include "../environment/BiomeSystem.h"
#include <glm/glm.hpp>
#include <array>
#include <vector>

// Forward declarations (global namespace)
class DayNightCycle;
class WeatherSystem;
class BiomeSystem;

namespace Forge {

// Forward declaration for Forge namespace class
class CameraController;

// ============================================================================
// Ambient Layer - One continuous ambient sound
// ============================================================================

struct AmbientLayer {
    SoundEffect effect = SoundEffect::WIND;
    SoundHandle handle;
    float targetVolume = 0.0f;
    float currentVolume = 0.0f;
    float fadeSpeed = 0.5f;  // Per second
    bool active = false;
    bool fadingOut = false;

    // Procedural regeneration timer (for non-looping sounds)
    float regenerateTimer = 0.0f;
    float regenerateInterval = 3.0f;  // Regenerate every N seconds
};

// ============================================================================
// Biome Soundscape Definition
// ============================================================================

struct BiomeSoundscape {
    BiomeType biome;

    // Base ambient layer (always playing when in this biome)
    SoundEffect baseAmbient = SoundEffect::WIND;
    float baseVolume = 0.3f;

    // Optional secondary layers
    SoundEffect secondaryAmbient = SoundEffect::COUNT;  // COUNT = none
    float secondaryVolume = 0.0f;

    // Day-specific sounds
    SoundEffect daySound = SoundEffect::COUNT;
    float dayVolume = 0.0f;

    // Night-specific sounds
    SoundEffect nightSound = SoundEffect::COUNT;
    float nightVolume = 0.0f;

    // Dawn chorus
    SoundEffect dawnSound = SoundEffect::COUNT;
    float dawnVolume = 0.0f;
};

// ============================================================================
// Time-of-Day Ranges (matching DayNightCycle)
// Note: Named TimeOfDayRanges to avoid conflict with ::TimeOfDay enum in PlanetTheme.h
// ============================================================================

namespace TimeOfDayRanges {
    constexpr float DAWN_START = 0.2f;
    constexpr float DAWN_END = 0.3f;
    constexpr float DAY_START = 0.3f;
    constexpr float DAY_END = 0.7f;
    constexpr float DUSK_START = 0.7f;
    constexpr float DUSK_END = 0.8f;
    // Night is < DAWN_START or > DUSK_END
}

// ============================================================================
// Ambient Soundscape Manager
// ============================================================================

class AmbientSoundscape {
public:
    // Maximum concurrent ambient layers (hard limit)
    static constexpr int MAX_AMBIENT_LAYERS = 4;
    static constexpr int MAX_WEATHER_LAYERS = 2;

    AmbientSoundscape();
    ~AmbientSoundscape() = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    void init(AudioManager* audio, BiomeSystem* biomes);

    // ========================================================================
    // Update
    // ========================================================================

    // Main update - call each frame
    void update(float deltaTime, const glm::vec3& listenerPosition,
                float timeOfDay, float weatherIntensity, float windSpeed);

    // Update with full system references
    void updateWithSystems(float deltaTime, CameraController* camera,
                          DayNightCycle* dayNight, WeatherSystem* weather);

    // ========================================================================
    // Configuration
    // ========================================================================

    // Master ambient volume
    void setAmbientVolume(float volume) { m_ambientVolume = volume; }
    float getAmbientVolume() const { return m_ambientVolume; }

    // Weather volume
    void setWeatherVolume(float volume) { m_weatherVolume = volume; }
    float getWeatherVolume() const { return m_weatherVolume; }

    // Enable/disable
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    // ========================================================================
    // Debug
    // ========================================================================

    // Get active layer info for UI
    struct LayerInfo {
        const char* name;
        float volume;
        bool active;
    };
    std::vector<LayerInfo> getActiveLayers() const;

    // Get current biome name
    const char* getCurrentBiomeName() const;

private:
    AudioManager* m_audio = nullptr;
    BiomeSystem* m_biomes = nullptr;
    ProceduralSynthesizer m_synthesizer;

    // Current state
    BiomeType m_currentBiome = BiomeType::GRASSLAND;
    float m_timeOfDay = 0.5f;
    float m_weatherIntensity = 0.0f;
    float m_windSpeed = 0.0f;

    // Ambient layers
    std::array<AmbientLayer, MAX_AMBIENT_LAYERS> m_ambientLayers;
    std::array<AmbientLayer, MAX_WEATHER_LAYERS> m_weatherLayers;

    // Configuration
    float m_ambientVolume = 0.5f;
    float m_weatherVolume = 0.6f;
    bool m_enabled = true;

    // Crossfade time for layer transitions
    static constexpr float CROSSFADE_TIME = 2.0f;

    // Biome soundscapes
    std::array<BiomeSoundscape, static_cast<size_t>(BiomeType::BIOME_COUNT)> m_biomeSoundscapes;

    // ========================================================================
    // Internal Methods
    // ========================================================================

    // Initialize biome sound mappings
    void initializeBiomeSoundscapes();

    // Get soundscape for current biome
    const BiomeSoundscape& getSoundscapeForBiome(BiomeType biome) const;

    // Update biome ambient sounds
    void updateBiomeAmbient(BiomeType biome, float timeOfDay);

    // Update weather layers
    void updateWeatherLayers(float rainIntensity, float windSpeed);

    // Update a single layer (handle fades)
    void updateLayer(AmbientLayer& layer, float deltaTime);

    // Start a layer
    void startLayer(AmbientLayer& layer, SoundEffect effect, float targetVolume);

    // Stop a layer with fade
    void stopLayer(AmbientLayer& layer);

    // Find an available layer slot (templated for different array sizes)
    template<size_t N>
    AmbientLayer* findAvailableLayer(std::array<AmbientLayer, N>& layers) {
        for (auto& layer : layers) {
            if (!layer.active && !layer.fadingOut) {
                return &layer;
            }
        }
        return nullptr;
    }

    // Find layer playing specific effect (templated for different array sizes)
    template<size_t N>
    AmbientLayer* findLayerWithEffect(std::array<AmbientLayer, N>& layers, SoundEffect effect) {
        for (auto& layer : layers) {
            if (layer.active && layer.effect == effect) {
                return &layer;
            }
        }
        return nullptr;
    }

    // Generate ambient sound buffer
    std::vector<int16_t> generateAmbientSound(SoundEffect effect);

    // Check if time is in dawn period
    bool isDawn(float timeOfDay) const;

    // Check if time is night
    bool isNight(float timeOfDay) const;
};

// ============================================================================
// Inline Implementation - Time Checks
// ============================================================================

inline bool AmbientSoundscape::isDawn(float timeOfDay) const {
    return timeOfDay >= TimeOfDayRanges::DAWN_START && timeOfDay <= TimeOfDayRanges::DAWN_END;
}

inline bool AmbientSoundscape::isNight(float timeOfDay) const {
    return timeOfDay < TimeOfDayRanges::DAWN_START || timeOfDay > TimeOfDayRanges::DUSK_END;
}

} // namespace Forge
