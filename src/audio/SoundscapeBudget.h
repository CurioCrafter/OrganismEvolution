#pragma once

// SoundscapeBudget - Voice pool management with hard limits and priority system
//
// Purpose: Prevent audio cacophony by strictly limiting simultaneous sounds
// and intelligently choosing which sounds play based on distance and importance
//
// Design:
// - Hard caps are NON-NEGOTIABLE (no "just one more" exceptions)
// - Priority combines distance (60%) and importance (40%)
// - Smooth fades prevent audio pops when swapping sounds
// - Spatial awareness: Use SpatialGrid to check for nearby vocalizing creatures

#include "ProceduralSynthesizer.h"
#include "../audio/AudioManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>

namespace Forge {

// ============================================================================
// Voice Slot - Represents one active sound
// ============================================================================

struct VoiceSlot {
    uint32_t soundId = 0;
    SoundHandle handle;

    // Sound properties
    SoundCategory category = SoundCategory::CREATURES;
    glm::vec3 position = glm::vec3(0.0f);
    float currentVolume = 0.0f;
    float targetVolume = 0.0f;

    // Fade state
    float fadeProgress = 1.0f;  // 0 = fading in, 1 = full volume
    bool fadingIn = false;
    bool fadingOut = false;

    // Priority info
    float distancePriority = 0.0f;
    float importancePriority = 0.0f;
    float totalPriority = 0.0f;

    // Timing
    float timeRemaining = 0.0f;
    float cooldownTime = 0.0f;  // After sound ends, cannot restart immediately

    // Associated creature (for tracking)
    uint32_t creatureId = 0;

    bool isActive() const { return handle.valid && !fadingOut; }
    bool isAvailable() const { return !handle.valid || (fadingOut && fadeProgress <= 0.0f); }
};

// ============================================================================
// Sound Request - Queued request to play a sound
// ============================================================================

struct SoundRequest {
    SoundEffect effect = SoundEffect::CREATURE_IDLE;
    SoundCategory category = SoundCategory::CREATURES;
    glm::vec3 position = glm::vec3(0.0f);
    float volume = 1.0f;
    float pitch = 1.0f;
    float duration = 1.0f;
    float importance = 0.5f;  // 0-1
    uint32_t creatureId = 0;  // Optional association

    // Custom generated audio buffer (if using procedural synthesis)
    std::vector<int16_t> customBuffer;
    bool useCustomBuffer = false;
};

// ============================================================================
// Importance Levels
// ============================================================================

namespace SoundImportance {
    constexpr float IDLE = 0.3f;
    constexpr float EATING = 0.25f;
    constexpr float MOVING = 0.2f;
    constexpr float ALERT = 0.7f;
    constexpr float HUNTING = 0.7f;
    constexpr float BEING_ATTACKED = 1.0f;
    constexpr float MATING = 0.6f;
    constexpr float DEATH = 0.9f;
    constexpr float BIRTH = 0.5f;

    // Ambient importance (lower than creatures)
    constexpr float AMBIENT_BIOME = 0.2f;
    constexpr float WEATHER = 0.3f;
    constexpr float UI = 0.1f;
}

// ============================================================================
// Vocalization Tracker - Prevents too many creatures vocalizing at once
// ============================================================================

class VocalizationTracker {
public:
    static constexpr int MAX_SIMULTANEOUS_NEARBY = 3;
    static constexpr float NEARBY_RADIUS = 20.0f;
    static constexpr float STAGGER_DELAY_MAX = 0.5f;  // Random delay 0-500ms

    // Check if a creature at this position can vocalize
    // Returns delay time (0 = can vocalize immediately, >0 = delay needed)
    float canVocalize(const glm::vec3& position) const;

    // Register that a creature started vocalizing
    void registerVocalization(const glm::vec3& position, float duration);

    // Update (remove expired vocalizations)
    void update(float deltaTime);

    // Clear all
    void clear();

private:
    struct ActiveVocalization {
        glm::vec3 position;
        float timeRemaining;
    };
    std::vector<ActiveVocalization> m_activeVocalizations;
};

// ============================================================================
// Soundscape Budget Manager
// ============================================================================

class SoundscapeBudget {
public:
    SoundscapeBudget();
    ~SoundscapeBudget() = default;

    // ========================================================================
    // Configuration
    // ========================================================================

    // Set voice limits (cannot exceed AudioConstants maximums)
    void setCreatureVoiceLimit(int limit);
    void setAmbientLayerLimit(int limit);
    void setWeatherSoundLimit(int limit);
    void setUISoundLimit(int limit);

    int getCreatureVoiceLimit() const { return m_creatureVoiceLimit; }
    int getAmbientLayerLimit() const { return m_ambientLayerLimit; }
    int getActiveCreatureVoices() const { return m_activeCreatureVoices; }
    int getActiveAmbientLayers() const { return m_activeAmbientLayers; }

    // Set listener position for distance calculations
    void setListenerPosition(const glm::vec3& position);

    // ========================================================================
    // Sound Request API
    // ========================================================================

    // Request to play a sound (may be denied or delayed based on budget)
    // Returns true if sound will play, false if rejected
    bool requestSound(const SoundRequest& request);

    // Request ambient layer (crossfades with existing)
    bool requestAmbientLayer(SoundEffect effect, float volume, float fadeTime = 1.0f);

    // Stop ambient layer with fade
    void stopAmbientLayer(SoundEffect effect, float fadeTime = 1.0f);

    // Request weather sound
    bool requestWeatherSound(SoundEffect effect, float volume, float fadeTime = 0.5f);
    void stopWeatherSound(SoundEffect effect, float fadeTime = 0.5f);

    // UI sounds (always play if below limit)
    bool playUISound(SoundEffect effect, float volume = 1.0f);

    // ========================================================================
    // Update
    // ========================================================================

    // Update all active sounds, fades, and cooldowns
    void update(float deltaTime, AudioManager* audioManager);

    // ========================================================================
    // Vocalization Control
    // ========================================================================

    // Check if a creature can vocalize at this position
    float getVocalizationDelay(const glm::vec3& position) const;

    // Register a vocalization (call when sound actually starts)
    void registerVocalization(const glm::vec3& position, float duration);

    // ========================================================================
    // Statistics
    // ========================================================================

    struct BudgetStats {
        int creatureVoicesActive = 0;
        int creatureVoicesLimit = 0;
        int ambientLayersActive = 0;
        int ambientLayersLimit = 0;
        int weatherSoundsActive = 0;
        int uiSoundsActive = 0;
        int requestsThisFrame = 0;
        int requestsRejected = 0;
        float averagePriority = 0.0f;
    };

    const BudgetStats& getStats() const { return m_stats; }

    // ========================================================================
    // Debug
    // ========================================================================

    // Get list of active sounds for debugging
    std::vector<std::pair<std::string, float>> getActiveSoundInfo() const;

private:
    // Voice pools by category
    std::vector<VoiceSlot> m_creatureVoices;
    std::vector<VoiceSlot> m_ambientVoices;
    std::vector<VoiceSlot> m_weatherVoices;
    std::vector<VoiceSlot> m_uiVoices;

    // Limits
    int m_creatureVoiceLimit = AudioConstants::MAX_CREATURE_VOICES;
    int m_ambientLayerLimit = AudioConstants::MAX_AMBIENT_LAYERS;
    int m_weatherSoundLimit = AudioConstants::MAX_WEATHER_SOUNDS;
    int m_uiSoundLimit = AudioConstants::MAX_UI_SOUNDS;

    // Current counts
    int m_activeCreatureVoices = 0;
    int m_activeAmbientLayers = 0;
    int m_activeWeatherSounds = 0;
    int m_activeUISounds = 0;

    // Listener position
    glm::vec3 m_listenerPosition = glm::vec3(0.0f);

    // Vocalization tracker
    VocalizationTracker m_vocalizationTracker;

    // Statistics
    BudgetStats m_stats;

    // Fade timing constants
    static constexpr float FADE_IN_TIME = 0.1f;   // 100ms fade in
    static constexpr float FADE_OUT_TIME = 0.2f;  // 200ms fade out

    // Request queue for priority sorting
    std::vector<SoundRequest> m_pendingRequests;

    // Sound ID counter
    uint32_t m_nextSoundId = 1;

    // ========================================================================
    // Internal Methods
    // ========================================================================

    // Calculate priority for a sound request
    float calculatePriority(const SoundRequest& request) const;

    // Calculate distance-based priority (1.0 = at listener, 0.0 = at max distance)
    float calculateDistancePriority(const glm::vec3& position) const;

    // Find an available slot, or the lowest priority slot to steal
    VoiceSlot* findSlotForRequest(std::vector<VoiceSlot>& pool, float requestPriority);

    // Update fade state for a voice
    void updateFade(VoiceSlot& slot, float deltaTime, AudioManager* audioManager);

    // Start a sound in a slot
    void startSoundInSlot(VoiceSlot& slot, const SoundRequest& request, AudioManager* audioManager);

    // Begin fade out for a slot
    void beginFadeOut(VoiceSlot& slot, AudioManager* audioManager);

    // Process pending requests after sorting by priority
    void processPendingRequests(AudioManager* audioManager);

    // Update voice counts
    void updateCounts();
};

// ============================================================================
// Inline Implementation - VocalizationTracker
// ============================================================================

inline float VocalizationTracker::canVocalize(const glm::vec3& position) const {
    int nearbyCount = 0;

    for (const auto& v : m_activeVocalizations) {
        float dist = glm::length(v.position - position);
        if (dist < NEARBY_RADIUS) {
            nearbyCount++;
        }
    }

    if (nearbyCount < MAX_SIMULTANEOUS_NEARBY) {
        return 0.0f;  // Can vocalize immediately
    }

    // Return a stagger delay
    return static_cast<float>(rand()) / RAND_MAX * STAGGER_DELAY_MAX;
}

inline void VocalizationTracker::registerVocalization(const glm::vec3& position, float duration) {
    m_activeVocalizations.push_back({position, duration});
}

inline void VocalizationTracker::update(float deltaTime) {
    for (auto it = m_activeVocalizations.begin(); it != m_activeVocalizations.end(); ) {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f) {
            it = m_activeVocalizations.erase(it);
        } else {
            ++it;
        }
    }
}

inline void VocalizationTracker::clear() {
    m_activeVocalizations.clear();
}

// ============================================================================
// Inline Implementation - SoundscapeBudget Simple Methods
// ============================================================================

inline void SoundscapeBudget::setCreatureVoiceLimit(int limit) {
    m_creatureVoiceLimit = std::min(limit, AudioConstants::MAX_CREATURE_VOICES);
}

inline void SoundscapeBudget::setAmbientLayerLimit(int limit) {
    m_ambientLayerLimit = std::min(limit, AudioConstants::MAX_AMBIENT_LAYERS);
}

inline void SoundscapeBudget::setWeatherSoundLimit(int limit) {
    m_weatherSoundLimit = std::min(limit, AudioConstants::MAX_WEATHER_SOUNDS);
}

inline void SoundscapeBudget::setUISoundLimit(int limit) {
    m_uiSoundLimit = std::min(limit, AudioConstants::MAX_UI_SOUNDS);
}

inline void SoundscapeBudget::setListenerPosition(const glm::vec3& position) {
    m_listenerPosition = position;
}

inline float SoundscapeBudget::getVocalizationDelay(const glm::vec3& position) const {
    return m_vocalizationTracker.canVocalize(position);
}

inline void SoundscapeBudget::registerVocalization(const glm::vec3& position, float duration) {
    m_vocalizationTracker.registerVocalization(position, duration);
}

inline float SoundscapeBudget::calculateDistancePriority(const glm::vec3& position) const {
    float distance = glm::length(position - m_listenerPosition);
    return 1.0f - std::min(distance / AudioConstants::MAX_AUDIO_DISTANCE, 1.0f);
}

inline float SoundscapeBudget::calculatePriority(const SoundRequest& request) const {
    float distancePriority = calculateDistancePriority(request.position);
    float importancePriority = request.importance;

    // Combine: 60% distance, 40% importance
    return distancePriority * 0.6f + importancePriority * 0.4f;
}

} // namespace Forge
