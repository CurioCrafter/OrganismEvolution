#include "SoundscapeBudget.h"
#include <algorithm>
#include <cmath>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

SoundscapeBudget::SoundscapeBudget() {
    // Pre-allocate voice pools
    m_creatureVoices.resize(AudioConstants::MAX_CREATURE_VOICES);
    m_ambientVoices.resize(AudioConstants::MAX_AMBIENT_LAYERS);
    m_weatherVoices.resize(AudioConstants::MAX_WEATHER_SOUNDS);
    m_uiVoices.resize(AudioConstants::MAX_UI_SOUNDS);

    m_pendingRequests.reserve(32);
}

// ============================================================================
// Sound Request API
// ============================================================================

bool SoundscapeBudget::requestSound(const SoundRequest& request) {
    m_stats.requestsThisFrame++;

    // Check if this is a creature sound
    if (request.category == SoundCategory::CREATURES) {
        // Check vocalization limit for nearby creatures
        float delay = m_vocalizationTracker.canVocalize(request.position);
        if (delay > 0.0f) {
            // Could queue with delay, but for simplicity, we reject
            m_stats.requestsRejected++;
            return false;
        }
    }

    // Calculate priority
    float priority = calculatePriority(request);

    // Quick reject if priority is too low and all slots are full
    std::vector<VoiceSlot>* pool = nullptr;
    int limit = 0;

    switch (request.category) {
        case SoundCategory::CREATURES:
            pool = &m_creatureVoices;
            limit = m_creatureVoiceLimit;
            break;
        case SoundCategory::AMBIENT:
            pool = &m_ambientVoices;
            limit = m_ambientLayerLimit;
            break;
        case SoundCategory::WEATHER:
            pool = &m_weatherVoices;
            limit = m_weatherSoundLimit;
            break;
        case SoundCategory::UI:
            pool = &m_uiVoices;
            limit = m_uiSoundLimit;
            break;
        default:
            m_stats.requestsRejected++;
            return false;
    }

    // Find lowest priority active sound
    float lowestPriority = priority;
    bool hasAvailable = false;

    for (int i = 0; i < limit && i < static_cast<int>(pool->size()); ++i) {
        if ((*pool)[i].isAvailable()) {
            hasAvailable = true;
            break;
        }
        if ((*pool)[i].isActive()) {
            lowestPriority = std::min(lowestPriority, (*pool)[i].totalPriority);
        }
    }

    if (!hasAvailable && lowestPriority >= priority) {
        // No room and we're not high enough priority to steal
        m_stats.requestsRejected++;
        return false;
    }

    // Queue the request for processing
    m_pendingRequests.push_back(request);
    return true;
}

bool SoundscapeBudget::requestAmbientLayer(SoundEffect effect, float volume, float fadeTime) {
    SoundRequest request;
    request.effect = effect;
    request.category = SoundCategory::AMBIENT;
    request.volume = volume;
    request.importance = SoundImportance::AMBIENT_BIOME;
    request.duration = -1.0f;  // Looping

    return requestSound(request);
}

void SoundscapeBudget::stopAmbientLayer(SoundEffect effect, float fadeTime) {
    for (auto& slot : m_ambientVoices) {
        if (slot.isActive()) {
            // Note: Would need to track effect type in slot for proper matching
            // For now, this is simplified
            beginFadeOut(slot, nullptr);
        }
    }
}

bool SoundscapeBudget::requestWeatherSound(SoundEffect effect, float volume, float fadeTime) {
    SoundRequest request;
    request.effect = effect;
    request.category = SoundCategory::WEATHER;
    request.volume = volume;
    request.importance = SoundImportance::WEATHER;
    request.duration = -1.0f;  // Looping

    return requestSound(request);
}

void SoundscapeBudget::stopWeatherSound(SoundEffect effect, float fadeTime) {
    for (auto& slot : m_weatherVoices) {
        if (slot.isActive()) {
            beginFadeOut(slot, nullptr);
        }
    }
}

bool SoundscapeBudget::playUISound(SoundEffect effect, float volume) {
    // UI sounds always play if under limit
    if (m_activeUISounds >= m_uiSoundLimit) {
        return false;
    }

    SoundRequest request;
    request.effect = effect;
    request.category = SoundCategory::UI;
    request.volume = volume;
    request.importance = SoundImportance::UI;
    request.duration = 0.5f;  // Short UI sounds

    m_pendingRequests.push_back(request);
    return true;
}

// ============================================================================
// Update
// ============================================================================

void SoundscapeBudget::update(float deltaTime, AudioManager* audioManager) {
    // Reset frame stats
    m_stats.requestsThisFrame = 0;
    m_stats.requestsRejected = 0;

    // Update vocalization tracker
    m_vocalizationTracker.update(deltaTime);

    // Update all active voices
    auto updatePool = [this, deltaTime, audioManager](std::vector<VoiceSlot>& pool) {
        for (auto& slot : pool) {
            if (slot.handle.valid) {
                updateFade(slot, deltaTime, audioManager);

                // Update time remaining
                if (slot.timeRemaining > 0.0f) {
                    slot.timeRemaining -= deltaTime;
                    if (slot.timeRemaining <= 0.0f && !slot.fadingOut) {
                        beginFadeOut(slot, audioManager);
                    }
                }

                // Update cooldown
                if (slot.cooldownTime > 0.0f) {
                    slot.cooldownTime -= deltaTime;
                }

                // Recalculate priority for active sounds
                if (slot.isActive()) {
                    slot.distancePriority = calculateDistancePriority(slot.position);
                    slot.totalPriority = slot.distancePriority * 0.6f + slot.importancePriority * 0.4f;
                }
            }
        }
    };

    updatePool(m_creatureVoices);
    updatePool(m_ambientVoices);
    updatePool(m_weatherVoices);
    updatePool(m_uiVoices);

    // Process pending requests
    processPendingRequests(audioManager);

    // Update counts
    updateCounts();

    // Update stats
    m_stats.creatureVoicesActive = m_activeCreatureVoices;
    m_stats.creatureVoicesLimit = m_creatureVoiceLimit;
    m_stats.ambientLayersActive = m_activeAmbientLayers;
    m_stats.ambientLayersLimit = m_ambientLayerLimit;
    m_stats.weatherSoundsActive = m_activeWeatherSounds;
    m_stats.uiSoundsActive = m_activeUISounds;
}

// ============================================================================
// Internal Methods
// ============================================================================

VoiceSlot* SoundscapeBudget::findSlotForRequest(std::vector<VoiceSlot>& pool, float requestPriority) {
    VoiceSlot* availableSlot = nullptr;
    VoiceSlot* lowestPrioritySlot = nullptr;
    float lowestPriority = requestPriority;

    for (auto& slot : pool) {
        // Prefer an available slot
        if (slot.isAvailable()) {
            if (!availableSlot) {
                availableSlot = &slot;
            }
        } else if (slot.isActive() && slot.totalPriority < lowestPriority) {
            lowestPriority = slot.totalPriority;
            lowestPrioritySlot = &slot;
        }
    }

    if (availableSlot) {
        return availableSlot;
    }

    return lowestPrioritySlot;  // May be null if request priority is lowest
}

void SoundscapeBudget::updateFade(VoiceSlot& slot, float deltaTime, AudioManager* audioManager) {
    if (slot.fadingIn) {
        slot.fadeProgress += deltaTime / FADE_IN_TIME;
        if (slot.fadeProgress >= 1.0f) {
            slot.fadeProgress = 1.0f;
            slot.fadingIn = false;
        }
        slot.currentVolume = slot.targetVolume * slot.fadeProgress;
    } else if (slot.fadingOut) {
        slot.fadeProgress -= deltaTime / FADE_OUT_TIME;
        if (slot.fadeProgress <= 0.0f) {
            slot.fadeProgress = 0.0f;
            slot.fadingOut = false;
            slot.currentVolume = 0.0f;

            // Stop the sound when fade completes
            if (audioManager && slot.handle.valid) {
                audioManager->stop(slot.handle);
                slot.handle = SoundHandle::invalid();
            }
        } else {
            slot.currentVolume = slot.targetVolume * slot.fadeProgress;
        }
    }

    // Update AudioManager with current volume
    // Note: Would need AudioManager to support setVolume(handle, volume)
}

void SoundscapeBudget::startSoundInSlot(VoiceSlot& slot, const SoundRequest& request, AudioManager* audioManager) {
    slot.soundId = m_nextSoundId++;
    slot.category = request.category;
    slot.position = request.position;
    slot.targetVolume = request.volume;
    slot.currentVolume = 0.0f;
    slot.fadeProgress = 0.0f;
    slot.fadingIn = true;
    slot.fadingOut = false;
    slot.distancePriority = calculateDistancePriority(request.position);
    slot.importancePriority = request.importance;
    slot.totalPriority = calculatePriority(request);
    slot.timeRemaining = request.duration;
    slot.cooldownTime = 0.0f;
    slot.creatureId = request.creatureId;

    // Start the sound
    if (audioManager) {
        if (request.category == SoundCategory::CREATURES && request.position != glm::vec3(0.0f)) {
            // 3D sound
            Sound3DParams params;
            params.position = request.position;
            params.volume = 0.0f;  // Start at 0, fade in
            params.pitch = request.pitch;
            params.loop = (request.duration < 0.0f);
            params.minDistance = AudioConstants::FULL_VOLUME_DISTANCE;
            params.maxDistance = AudioConstants::MAX_AUDIO_DISTANCE;

            slot.handle = audioManager->play3D(request.effect, params);
        } else {
            // 2D sound
            slot.handle = audioManager->play(request.effect, 0.0f, request.pitch, request.duration < 0.0f);
        }
    }

    // Register vocalization if creature sound
    if (request.category == SoundCategory::CREATURES && request.duration > 0.0f) {
        m_vocalizationTracker.registerVocalization(request.position, request.duration);
    }
}

void SoundscapeBudget::beginFadeOut(VoiceSlot& slot, AudioManager* audioManager) {
    if (!slot.fadingOut) {
        slot.fadingOut = true;
        slot.fadingIn = false;
        slot.fadeProgress = slot.currentVolume / std::max(slot.targetVolume, 0.001f);
    }
}

void SoundscapeBudget::processPendingRequests(AudioManager* audioManager) {
    if (m_pendingRequests.empty()) return;

    // Sort by priority (highest first)
    std::sort(m_pendingRequests.begin(), m_pendingRequests.end(),
        [this](const SoundRequest& a, const SoundRequest& b) {
            return calculatePriority(a) > calculatePriority(b);
        });

    // Process each request
    for (const auto& request : m_pendingRequests) {
        std::vector<VoiceSlot>* pool = nullptr;
        int limit = 0;

        switch (request.category) {
            case SoundCategory::CREATURES:
                pool = &m_creatureVoices;
                limit = m_creatureVoiceLimit;
                break;
            case SoundCategory::AMBIENT:
                pool = &m_ambientVoices;
                limit = m_ambientLayerLimit;
                break;
            case SoundCategory::WEATHER:
                pool = &m_weatherVoices;
                limit = m_weatherSoundLimit;
                break;
            case SoundCategory::UI:
                pool = &m_uiVoices;
                limit = m_uiSoundLimit;
                break;
            default:
                continue;
        }

        float priority = calculatePriority(request);

        // Find a slot
        VoiceSlot* slot = nullptr;
        for (int i = 0; i < limit && i < static_cast<int>(pool->size()); ++i) {
            if ((*pool)[i].isAvailable()) {
                slot = &(*pool)[i];
                break;
            }
        }

        // If no available slot, try to steal lowest priority
        if (!slot) {
            float lowestPriority = priority;
            for (int i = 0; i < limit && i < static_cast<int>(pool->size()); ++i) {
                if ((*pool)[i].isActive() && (*pool)[i].totalPriority < lowestPriority) {
                    lowestPriority = (*pool)[i].totalPriority;
                    slot = &(*pool)[i];
                }
            }

            // Begin fade out on stolen slot
            if (slot && slot->isActive()) {
                beginFadeOut(*slot, audioManager);
                // Need to wait for fade to complete before reusing
                slot = nullptr;  // Don't use this frame
            }
        }

        if (slot) {
            startSoundInSlot(*slot, request, audioManager);
        }
    }

    m_pendingRequests.clear();
}

void SoundscapeBudget::updateCounts() {
    m_activeCreatureVoices = 0;
    m_activeAmbientLayers = 0;
    m_activeWeatherSounds = 0;
    m_activeUISounds = 0;

    for (const auto& slot : m_creatureVoices) {
        if (slot.isActive()) m_activeCreatureVoices++;
    }
    for (const auto& slot : m_ambientVoices) {
        if (slot.isActive()) m_activeAmbientLayers++;
    }
    for (const auto& slot : m_weatherVoices) {
        if (slot.isActive()) m_activeWeatherSounds++;
    }
    for (const auto& slot : m_uiVoices) {
        if (slot.isActive()) m_activeUISounds++;
    }
}

std::vector<std::pair<std::string, float>> SoundscapeBudget::getActiveSoundInfo() const {
    std::vector<std::pair<std::string, float>> info;

    auto addFromPool = [&info](const std::vector<VoiceSlot>& pool, const char* prefix) {
        int index = 0;
        for (const auto& slot : pool) {
            if (slot.isActive()) {
                std::string name = std::string(prefix) + std::to_string(index);
                info.push_back({name, slot.totalPriority});
            }
            index++;
        }
    };

    addFromPool(m_creatureVoices, "Creature_");
    addFromPool(m_ambientVoices, "Ambient_");
    addFromPool(m_weatherVoices, "Weather_");
    addFromPool(m_uiVoices, "UI_");

    return info;
}

} // namespace Forge
