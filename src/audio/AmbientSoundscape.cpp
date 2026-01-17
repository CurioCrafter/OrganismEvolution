#include "AmbientSoundscape.h"
#include "../core/DayNightCycle.h"
#include "../environment/WeatherSystem.h"
#include "../graphics/CameraController.h"

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

AmbientSoundscape::AmbientSoundscape() {
    initializeBiomeSoundscapes();
}

// ============================================================================
// Initialization
// ============================================================================

void AmbientSoundscape::init(AudioManager* audio, BiomeSystem* biomes) {
    m_audio = audio;
    m_biomes = biomes;
}

void AmbientSoundscape::initializeBiomeSoundscapes() {
    // Initialize all biome soundscapes with appropriate ambient sounds

    // Water biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::DEEP_OCEAN)] = {
        BiomeType::DEEP_OCEAN,
        SoundEffect::UNDERWATER_AMBIENT, 0.4f,
        SoundEffect::WATER_FLOW, 0.2f,
        SoundEffect::COUNT, 0.0f,       // No day-specific
        SoundEffect::COUNT, 0.0f,       // No night-specific
        SoundEffect::COUNT, 0.0f        // No dawn
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::OCEAN)] = {
        BiomeType::OCEAN,
        SoundEffect::WATER_FLOW, 0.4f,
        SoundEffect::WIND, 0.2f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::SHALLOW_WATER)] = {
        BiomeType::SHALLOW_WATER,
        SoundEffect::WATER_FLOW, 0.35f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::BIRD_CHIRP, 0.15f,  // Shore birds during day
        SoundEffect::FROGS, 0.2f,        // Frogs at night
        SoundEffect::BIRD_SONG, 0.3f     // Dawn chorus
    };

    // Coastal biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::BEACH_SANDY)] = {
        BiomeType::BEACH_SANDY,
        SoundEffect::WATER_FLOW, 0.35f,   // Waves
        SoundEffect::WIND, 0.2f,
        SoundEffect::BIRD_CHIRP, 0.2f,    // Seabirds
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::MANGROVE)] = {
        BiomeType::MANGROVE,
        SoundEffect::WATER_FLOW, 0.25f,
        SoundEffect::INSECT_BUZZ, 0.15f,
        SoundEffect::BIRD_CHIRP, 0.2f,
        SoundEffect::FROGS, 0.25f,
        SoundEffect::BIRD_SONG, 0.3f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::SWAMP)] = {
        BiomeType::SWAMP,
        SoundEffect::WATER_FLOW, 0.15f,
        SoundEffect::INSECT_BUZZ, 0.2f,
        SoundEffect::BIRD_CHIRP, 0.15f,
        SoundEffect::FROGS, 0.35f,        // Lots of frogs at night
        SoundEffect::BIRD_SONG, 0.25f
    };

    // Forest biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::TEMPERATE_FOREST)] = {
        BiomeType::TEMPERATE_FOREST,
        SoundEffect::WIND, 0.2f,          // Wind in leaves
        SoundEffect::GRASS_RUSTLE, 0.15f,
        SoundEffect::BIRD_CHIRP, 0.25f,   // Birds during day
        SoundEffect::CRICKETS, 0.25f,     // Crickets at night
        SoundEffect::BIRD_SONG, 0.4f      // Strong dawn chorus
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::TROPICAL_RAINFOREST)] = {
        BiomeType::TROPICAL_RAINFOREST,
        SoundEffect::WIND, 0.15f,
        SoundEffect::INSECT_BUZZ, 0.2f,
        SoundEffect::BIRD_CHIRP, 0.3f,    // Constant bird sounds
        SoundEffect::FROGS, 0.25f,
        SoundEffect::BIRD_SONG, 0.4f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::BOREAL_FOREST)] = {
        BiomeType::BOREAL_FOREST,
        SoundEffect::WIND, 0.3f,
        SoundEffect::TREE_CREAK, 0.1f,
        SoundEffect::BIRD_CHIRP, 0.15f,
        SoundEffect::COUNT, 0.0f,          // Quieter at night
        SoundEffect::BIRD_SONG, 0.25f
    };

    // Grassland biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::GRASSLAND)] = {
        BiomeType::GRASSLAND,
        SoundEffect::WIND, 0.3f,
        SoundEffect::GRASS_RUSTLE, 0.25f,
        SoundEffect::BIRD_CHIRP, 0.2f,
        SoundEffect::CRICKETS, 0.3f,
        SoundEffect::BIRD_SONG, 0.35f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::SAVANNA)] = {
        BiomeType::SAVANNA,
        SoundEffect::WIND, 0.35f,
        SoundEffect::GRASS_RUSTLE, 0.2f,
        SoundEffect::BIRD_CHIRP, 0.15f,
        SoundEffect::CRICKETS, 0.25f,
        SoundEffect::BIRD_SONG, 0.3f
    };

    // Desert biomes - mostly silence with occasional wind
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::DESERT_HOT)] = {
        BiomeType::DESERT_HOT,
        SoundEffect::WIND, 0.15f,          // Minimal, 90% silence
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::DESERT_COLD)] = {
        BiomeType::DESERT_COLD,
        SoundEffect::WIND, 0.25f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    // Mountain biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::ALPINE_MEADOW)] = {
        BiomeType::ALPINE_MEADOW,
        SoundEffect::WIND, 0.4f,
        SoundEffect::GRASS_RUSTLE, 0.15f,
        SoundEffect::BIRD_CHIRP, 0.15f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::BIRD_SONG, 0.2f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::ROCKY_HIGHLANDS)] = {
        BiomeType::ROCKY_HIGHLANDS,
        SoundEffect::WIND, 0.45f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::BIRD_CHIRP, 0.1f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::TUNDRA)] = {
        BiomeType::TUNDRA,
        SoundEffect::WIND, 0.5f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    m_biomeSoundscapes[static_cast<size_t>(BiomeType::GLACIER)] = {
        BiomeType::GLACIER,
        SoundEffect::WIND, 0.4f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    // Wetland biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::WETLAND)] = {
        BiomeType::WETLAND,
        SoundEffect::WATER_FLOW, 0.2f,
        SoundEffect::INSECT_BUZZ, 0.15f,
        SoundEffect::BIRD_CHIRP, 0.2f,
        SoundEffect::FROGS, 0.3f,
        SoundEffect::BIRD_SONG, 0.3f
    };

    // Special biomes
    m_biomeSoundscapes[static_cast<size_t>(BiomeType::VOLCANIC)] = {
        BiomeType::VOLCANIC,
        SoundEffect::WIND, 0.3f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f,
        SoundEffect::COUNT, 0.0f
    };

    // Default for unmapped biomes
    for (size_t i = 0; i < m_biomeSoundscapes.size(); ++i) {
        if (m_biomeSoundscapes[i].baseAmbient == SoundEffect::COUNT) {
            m_biomeSoundscapes[i] = {
                static_cast<BiomeType>(i),
                SoundEffect::WIND, 0.25f,
                SoundEffect::COUNT, 0.0f,
                SoundEffect::COUNT, 0.0f,
                SoundEffect::COUNT, 0.0f,
                SoundEffect::COUNT, 0.0f
            };
        }
    }
}

// ============================================================================
// Update
// ============================================================================

void AmbientSoundscape::update(float deltaTime, const glm::vec3& listenerPosition,
                               float timeOfDay, float weatherIntensity, float windSpeed) {
    if (!m_enabled || !m_audio) return;

    m_timeOfDay = timeOfDay;
    m_weatherIntensity = weatherIntensity;
    m_windSpeed = windSpeed;

    // Query biome at listener position
    if (m_biomes) {
        BiomeQuery query = m_biomes->queryBiome(listenerPosition.x, listenerPosition.z);
        m_currentBiome = query.biome;
    }

    // Update biome ambient sounds
    updateBiomeAmbient(m_currentBiome, timeOfDay);

    // Update weather layers
    updateWeatherLayers(weatherIntensity, windSpeed);

    // Update all layer fades
    for (auto& layer : m_ambientLayers) {
        updateLayer(layer, deltaTime);
    }
    for (auto& layer : m_weatherLayers) {
        updateLayer(layer, deltaTime);
    }
}

void AmbientSoundscape::updateWithSystems(float deltaTime, CameraController* camera,
                                          DayNightCycle* dayNight, WeatherSystem* weather) {
    if (!camera) return;

    glm::vec3 listenerPos = camera->getPosition();

    float timeOfDay = 0.5f;
    if (dayNight) {
        timeOfDay = dayNight->dayTime;
    }

    float weatherIntensity = 0.0f;
    float windSpeed = 0.0f;
    if (weather) {
        const WeatherState& state = weather->getCurrentWeather();
        weatherIntensity = state.precipitationIntensity;
        windSpeed = state.windStrength * 20.0f;  // Scale to reasonable range
    }

    update(deltaTime, listenerPos, timeOfDay, weatherIntensity, windSpeed);
}

void AmbientSoundscape::setEnabled(bool enabled) {
    m_enabled = enabled;

    if (!enabled) {
        // Fade out all layers
        for (auto& layer : m_ambientLayers) {
            if (layer.active) stopLayer(layer);
        }
        for (auto& layer : m_weatherLayers) {
            if (layer.active) stopLayer(layer);
        }
    }
}

// ============================================================================
// Debug
// ============================================================================

std::vector<AmbientSoundscape::LayerInfo> AmbientSoundscape::getActiveLayers() const {
    std::vector<LayerInfo> info;

    auto addLayers = [&info](const auto& layers, const char* prefix) {
        int i = 0;
        for (const auto& layer : layers) {
            if (layer.active) {
                char name[32];
                snprintf(name, sizeof(name), "%s%d", prefix, i);
                info.push_back({name, layer.currentVolume, true});
            }
            i++;
        }
    };

    addLayers(m_ambientLayers, "Ambient_");
    addLayers(m_weatherLayers, "Weather_");

    return info;
}

const char* AmbientSoundscape::getCurrentBiomeName() const {
    return biomeToString(m_currentBiome);
}

// ============================================================================
// Internal Methods
// ============================================================================

const BiomeSoundscape& AmbientSoundscape::getSoundscapeForBiome(BiomeType biome) const {
    size_t index = static_cast<size_t>(biome);
    if (index < m_biomeSoundscapes.size()) {
        return m_biomeSoundscapes[index];
    }
    return m_biomeSoundscapes[static_cast<size_t>(BiomeType::GRASSLAND)];
}

void AmbientSoundscape::updateBiomeAmbient(BiomeType biome, float timeOfDay) {
    const BiomeSoundscape& soundscape = getSoundscapeForBiome(biome);

    // Always play base ambient
    AmbientLayer* baseLayer = findLayerWithEffect(m_ambientLayers, soundscape.baseAmbient);
    if (!baseLayer && soundscape.baseAmbient != SoundEffect::COUNT) {
        baseLayer = findAvailableLayer(m_ambientLayers);
        if (baseLayer) {
            startLayer(*baseLayer, soundscape.baseAmbient,
                      soundscape.baseVolume * m_ambientVolume);
        }
    } else if (baseLayer) {
        baseLayer->targetVolume = soundscape.baseVolume * m_ambientVolume;
    }

    // Secondary ambient
    if (soundscape.secondaryAmbient != SoundEffect::COUNT) {
        AmbientLayer* secondaryLayer = findLayerWithEffect(m_ambientLayers, soundscape.secondaryAmbient);
        if (!secondaryLayer) {
            secondaryLayer = findAvailableLayer(m_ambientLayers);
            if (secondaryLayer) {
                startLayer(*secondaryLayer, soundscape.secondaryAmbient,
                          soundscape.secondaryVolume * m_ambientVolume);
            }
        } else {
            secondaryLayer->targetVolume = soundscape.secondaryVolume * m_ambientVolume;
        }
    }

    // Time-of-day specific sounds
    bool isDawnTime = isDawn(timeOfDay);
    bool isNightTime = isNight(timeOfDay);

    // Dawn chorus
    if (isDawnTime && soundscape.dawnSound != SoundEffect::COUNT) {
        AmbientLayer* dawnLayer = findLayerWithEffect(m_ambientLayers, soundscape.dawnSound);
        if (!dawnLayer) {
            dawnLayer = findAvailableLayer(m_ambientLayers);
            if (dawnLayer) {
                startLayer(*dawnLayer, soundscape.dawnSound,
                          soundscape.dawnVolume * m_ambientVolume);
            }
        }
    } else {
        // Fade out dawn sounds if not dawn
        AmbientLayer* dawnLayer = findLayerWithEffect(m_ambientLayers, soundscape.dawnSound);
        if (dawnLayer) {
            stopLayer(*dawnLayer);
        }
    }

    // Night sounds
    if (isNightTime && soundscape.nightSound != SoundEffect::COUNT) {
        AmbientLayer* nightLayer = findLayerWithEffect(m_ambientLayers, soundscape.nightSound);
        if (!nightLayer) {
            nightLayer = findAvailableLayer(m_ambientLayers);
            if (nightLayer) {
                startLayer(*nightLayer, soundscape.nightSound,
                          soundscape.nightVolume * m_ambientVolume);
            }
        }
    } else {
        // Fade out night sounds if not night
        AmbientLayer* nightLayer = findLayerWithEffect(m_ambientLayers, soundscape.nightSound);
        if (nightLayer && !isNightTime) {
            stopLayer(*nightLayer);
        }
    }

    // Day sounds (when not dawn and not night)
    if (!isDawnTime && !isNightTime && soundscape.daySound != SoundEffect::COUNT) {
        AmbientLayer* dayLayer = findLayerWithEffect(m_ambientLayers, soundscape.daySound);
        if (!dayLayer) {
            dayLayer = findAvailableLayer(m_ambientLayers);
            if (dayLayer) {
                startLayer(*dayLayer, soundscape.daySound,
                          soundscape.dayVolume * m_ambientVolume);
            }
        }
    } else {
        AmbientLayer* dayLayer = findLayerWithEffect(m_ambientLayers, soundscape.daySound);
        if (dayLayer && (isDawnTime || isNightTime)) {
            stopLayer(*dayLayer);
        }
    }
}

void AmbientSoundscape::updateWeatherLayers(float rainIntensity, float windSpeed) {
    // Rain sounds
    if (rainIntensity > 0.3f) {
        SoundEffect rainEffect = (rainIntensity > 0.6f) ?
            SoundEffect::RAIN_HEAVY : SoundEffect::RAIN_LIGHT;

        AmbientLayer* rainLayer = findLayerWithEffect(m_weatherLayers, rainEffect);
        if (!rainLayer) {
            // Check if we have a different rain playing
            for (auto& layer : m_weatherLayers) {
                if (layer.active && (layer.effect == SoundEffect::RAIN_LIGHT ||
                    layer.effect == SoundEffect::RAIN_HEAVY)) {
                    stopLayer(layer);
                }
            }

            rainLayer = findAvailableLayer(m_weatherLayers);
            if (rainLayer) {
                float volume = (rainIntensity - 0.3f) / 0.7f * m_weatherVolume;
                startLayer(*rainLayer, rainEffect, volume);
            }
        } else {
            rainLayer->targetVolume = (rainIntensity - 0.3f) / 0.7f * m_weatherVolume;
        }
    } else {
        // Fade out rain
        for (auto& layer : m_weatherLayers) {
            if (layer.effect == SoundEffect::RAIN_LIGHT ||
                layer.effect == SoundEffect::RAIN_HEAVY) {
                stopLayer(layer);
            }
        }
    }

    // Wind sounds scale with wind speed
    if (windSpeed > 5.0f) {
        AmbientLayer* windLayer = findLayerWithEffect(m_weatherLayers, SoundEffect::WIND);
        if (!windLayer) {
            windLayer = findAvailableLayer(m_weatherLayers);
            if (windLayer) {
                float volume = std::min((windSpeed - 5.0f) / 15.0f, 1.0f) * m_weatherVolume;
                startLayer(*windLayer, SoundEffect::WIND, volume);
            }
        } else {
            windLayer->targetVolume = std::min((windSpeed - 5.0f) / 15.0f, 1.0f) * m_weatherVolume;
        }
    } else {
        // Fade out wind
        AmbientLayer* windLayer = findLayerWithEffect(m_weatherLayers, SoundEffect::WIND);
        if (windLayer) {
            stopLayer(*windLayer);
        }
    }
}

void AmbientSoundscape::updateLayer(AmbientLayer& layer, float deltaTime) {
    if (!layer.active) return;

    // Handle fade
    if (layer.currentVolume < layer.targetVolume) {
        layer.currentVolume = std::min(layer.currentVolume + layer.fadeSpeed * deltaTime,
                                       layer.targetVolume);
    } else if (layer.currentVolume > layer.targetVolume) {
        layer.currentVolume = std::max(layer.currentVolume - layer.fadeSpeed * deltaTime,
                                       layer.targetVolume);

        // Stop when fully faded out
        if (layer.fadingOut && layer.currentVolume <= 0.0f) {
            if (m_audio && layer.handle.valid) {
                m_audio->stop(layer.handle);
            }
            layer.active = false;
            layer.fadingOut = false;
            layer.handle = SoundHandle::invalid();
        }
    }

    // Regenerate procedural sounds when timer expires
    if (!layer.fadingOut && layer.active) {
        layer.regenerateTimer -= deltaTime;
        if (layer.regenerateTimer <= 0.0f) {
            layer.regenerateTimer = layer.regenerateInterval;

            // Generate new sound buffer and restart
            auto buffer = generateAmbientSound(layer.effect);
            if (!buffer.empty() && m_audio) {
                // Stop old sound
                if (layer.handle.valid) {
                    m_audio->stop(layer.handle);
                }

                // Play new sound
                layer.handle = m_audio->playBuffer(buffer, glm::vec3(0.0f), layer.currentVolume);
            }
        }
    }
}

void AmbientSoundscape::startLayer(AmbientLayer& layer, SoundEffect effect, float targetVolume) {
    layer.effect = effect;
    layer.targetVolume = targetVolume;
    layer.currentVolume = 0.0f;  // Start silent, fade in
    layer.fadeSpeed = 1.0f / CROSSFADE_TIME;
    layer.active = true;
    layer.fadingOut = false;
    layer.regenerateTimer = 0.0f;  // Generate immediately
    layer.regenerateInterval = 3.0f;  // Default regeneration interval

    // Generate initial sound
    auto buffer = generateAmbientSound(effect);
    if (!buffer.empty() && m_audio) {
        layer.handle = m_audio->playBuffer(buffer, glm::vec3(0.0f), 0.0f);
    }
}

void AmbientSoundscape::stopLayer(AmbientLayer& layer) {
    if (!layer.active) return;

    layer.targetVolume = 0.0f;
    layer.fadingOut = true;
    layer.fadeSpeed = 1.0f / CROSSFADE_TIME;
}

// Note: findAvailableLayer and findLayerWithEffect are now templated in header

std::vector<int16_t> AmbientSoundscape::generateAmbientSound(SoundEffect effect) {
    SynthParams params;

    switch (effect) {
        case SoundEffect::WIND:
            params = m_synthesizer.createWind(0.5f);
            break;
        case SoundEffect::RAIN_LIGHT:
            params = m_synthesizer.createRainAmbient(0.3f);
            break;
        case SoundEffect::RAIN_HEAVY:
            params = m_synthesizer.createRainAmbient(0.8f);
            break;
        case SoundEffect::WATER_FLOW:
            params = m_synthesizer.createWaterFlow(0.5f);
            break;
        case SoundEffect::UNDERWATER_AMBIENT:
            params = m_synthesizer.createUnderwaterAmbient();
            break;
        case SoundEffect::CRICKETS:
            params = m_synthesizer.createCrickets();
            break;
        case SoundEffect::FROGS:
            params = m_synthesizer.createFrogChorus();
            break;
        case SoundEffect::BIRD_CHIRP:
            params = m_synthesizer.createBirdChirp(1.0f);
            break;
        case SoundEffect::BIRD_SONG:
            params = m_synthesizer.createBirdSong(1.0f);
            break;
        case SoundEffect::INSECT_BUZZ:
            params = m_synthesizer.createInsectBuzz(50.0f);
            break;
        case SoundEffect::GRASS_RUSTLE:
            params = m_synthesizer.createWind(0.3f);  // Reuse wind with lower intensity
            params.filterCutoff = 3000.0f;  // Higher cutoff for rustling
            break;
        case SoundEffect::TREE_CREAK:
            // Low creaking sound
            params.voiceType = VoiceType::ADDITIVE;
            params.baseFrequency = 60.0f;
            params.duration = 1.5f;
            params.volume = 0.2f;
            params.envelope = Envelope::soft();
            params.harmonic2 = 0.4f;
            params.harmonic3 = 0.2f;
            break;
        default:
            params = m_synthesizer.createWind(0.3f);
            break;
    }

    return m_synthesizer.generate(params);
}

} // namespace Forge
