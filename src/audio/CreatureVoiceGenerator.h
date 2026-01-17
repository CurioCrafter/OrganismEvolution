#pragma once

// CreatureVoiceGenerator - Maps creature genome traits to procedural vocalizations
//
// Design philosophy:
// - Sound emerges from genetics, not random selection
// - Size determines pitch (small=high, large=low)
// - Speed determines call rhythm/tempo
// - Type determines voice character (herbivore coos, carnivore growls, etc.)
// - All outputs are musically constrained (pentatonic scale)
//
// Sound events with cooldowns prevent audio spam

#include "AudioManager.h"
#include "ProceduralSynthesizer.h"
#include "SoundscapeBudget.h"
#include "../entities/Creature.h"
#include "../entities/Genome.h"
#include "../entities/CreatureType.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <random>

namespace Forge {

// Forward declarations
class CreatureManager;
class SpatialGrid;

// ============================================================================
// Sound Event Types
// ============================================================================

enum class CreatureSoundEvent {
    IDLE,           // Occasional ambient sounds (very quiet)
    ALERT,          // Detecting predator/threat
    MATING_CALL,    // During reproduction check
    EATING,         // Consuming food (subtle)
    PAIN,           // Taking damage
    DEATH,          // Brief death sound
    BIRTH,          // New creature spawned
    HUNTING,        // Carnivore pursuing prey
    SWIMMING,       // Aquatic movement
    FLYING,         // Wing flaps
    ATTACKING       // Combat sounds
};

// ============================================================================
// Vocalization State - Per-creature audio state
// ============================================================================

struct CreatureVocalizationState {
    // Cooldown timers by event type (prevents spam)
    float idleCooldown = 0.0f;
    float alertCooldown = 0.0f;
    float matingCooldown = 0.0f;
    float eatingCooldown = 0.0f;
    float painCooldown = 0.0f;
    float movementCooldown = 0.0f;

    // Last played sound handle
    SoundHandle lastSound;

    // Vocalization parameters derived from genome
    float basePitch = 440.0f;
    float tempo = 1.0f;
    VoiceType voiceCharacter = VoiceType::SINE;
};

// ============================================================================
// Cooldown Configuration
// ============================================================================

namespace VocalizationCooldowns {
    // Minimum time between same sound type (seconds)
    constexpr float IDLE_MIN = 5.0f;
    constexpr float IDLE_MAX = 15.0f;
    constexpr float ALERT = 3.0f;
    constexpr float MATING = 10.0f;
    constexpr float EATING = 2.0f;
    constexpr float PAIN = 0.5f;
    constexpr float DEATH = 0.0f;  // Always plays
    constexpr float MOVEMENT = 0.3f;
    constexpr float ATTACK = 0.5f;

    // Idle sound probability (10% chance when off cooldown)
    constexpr float IDLE_PROBABILITY = 0.10f;

    // Eating sound probability (50% volume)
    constexpr float EATING_VOLUME_MULTIPLIER = 0.5f;
}

// ============================================================================
// Creature Voice Generator
// ============================================================================

class CreatureVoiceGenerator {
public:
    CreatureVoiceGenerator();
    ~CreatureVoiceGenerator() = default;

    // ========================================================================
    // Initialization
    // ========================================================================

    void init(AudioManager* audio, SoundscapeBudget* budget);

    // ========================================================================
    // Vocalization API
    // ========================================================================

    // Try to play a sound event for a creature
    // Returns true if sound was played (not on cooldown, budget allows)
    bool tryPlaySound(Creature& creature, CreatureSoundEvent event);

    // Force play a sound (ignores cooldown, respects budget)
    bool forcePlaySound(Creature& creature, CreatureSoundEvent event);

    // ========================================================================
    // Update
    // ========================================================================

    // Update all creatures' vocalization states
    void update(float deltaTime, const std::vector<std::unique_ptr<Creature>>& creatures);

    // Update cooldowns only (call per-frame)
    void updateCooldowns(float deltaTime);

    // ========================================================================
    // Configuration
    // ========================================================================

    // Global volume multiplier for creature sounds
    void setCreatureVolume(float volume) { m_creatureVolume = volume; }
    float getCreatureVolume() const { return m_creatureVolume; }

    // Enable/disable creature sounds entirely
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

    // ========================================================================
    // Genome-to-Sound Mapping
    // ========================================================================

    // Get voice parameters from genome
    static float getBasePitchFromGenome(const Genome& genome);
    static float getTempoFromGenome(const Genome& genome);
    static VoiceType getVoiceTypeFromCreatureType(CreatureType type);

private:
    AudioManager* m_audio = nullptr;
    SoundscapeBudget* m_budget = nullptr;
    ProceduralSynthesizer m_synthesizer;

    // Per-creature vocalization state (keyed by creature ID)
    std::unordered_map<int, CreatureVocalizationState> m_creatureStates;

    // Configuration
    float m_creatureVolume = 1.0f;
    bool m_enabled = true;

    // Random number generator
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_randomDist{0.0f, 1.0f};

    // ========================================================================
    // Internal Methods
    // ========================================================================

    // Get or create vocalization state for creature
    CreatureVocalizationState& getState(Creature& creature);

    // Generate sound buffer for event
    std::vector<int16_t> generateSoundForEvent(Creature& creature, CreatureSoundEvent event);

    // Check if cooldown allows sound
    bool checkCooldown(CreatureVocalizationState& state, CreatureSoundEvent event);

    // Reset cooldown after playing sound
    void resetCooldown(CreatureVocalizationState& state, CreatureSoundEvent event);

    // Get volume for event type
    float getVolumeForEvent(CreatureSoundEvent event) const;

    // Get importance for budget priority
    float getImportanceForEvent(CreatureSoundEvent event) const;

    // Play the actual sound
    bool playSound(Creature& creature, CreatureVocalizationState& state,
                   CreatureSoundEvent event, const std::vector<int16_t>& buffer);
};

// ============================================================================
// Inline Implementation - Genome Mappings
// ============================================================================

inline float CreatureVoiceGenerator::getBasePitchFromGenome(const Genome& genome) {
    // Map size (0.5-2.0) to pitch (800Hz-80Hz)
    // Smaller creatures = higher pitch (like in nature)
    return ProceduralSynthesizer::sizeToFrequency(genome.size);
}

inline float CreatureVoiceGenerator::getTempoFromGenome(const Genome& genome) {
    // Map speed (5-20) to tempo (0.5-2.0 rhythm rate)
    return ProceduralSynthesizer::speedToTempo(genome.speed);
}

inline VoiceType CreatureVoiceGenerator::getVoiceTypeFromCreatureType(CreatureType type) {
    // Each creature type has a characteristic voice
    switch (type) {
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::FRUGIVORE:
            return VoiceType::TRIANGLE;  // Soft, mellow coos/bleats

        case CreatureType::SMALL_PREDATOR:
            return VoiceType::ADDITIVE;  // Rich yips

        case CreatureType::APEX_PREDATOR:
        case CreatureType::OMNIVORE:
            return VoiceType::NOISE_FILTERED;  // Growls with filtered noise

        case CreatureType::FLYING_BIRD:
        case CreatureType::AERIAL_PREDATOR:
            return VoiceType::FM_BELL;  // Melodic whistles

        case CreatureType::FLYING_INSECT:
            return VoiceType::PULSE;  // Buzzing

        case CreatureType::AQUATIC_HERBIVORE:
        case CreatureType::AQUATIC:
            return VoiceType::SINE;  // Pure underwater tones

        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AQUATIC_APEX:
            return VoiceType::ADDITIVE;  // Rich underwater sounds

        default:
            return VoiceType::SINE;
    }
}

inline float CreatureVoiceGenerator::getVolumeForEvent(CreatureSoundEvent event) const {
    switch (event) {
        case CreatureSoundEvent::IDLE:
            return 0.25f * m_creatureVolume;  // Very quiet
        case CreatureSoundEvent::EATING:
            return 0.15f * m_creatureVolume * VocalizationCooldowns::EATING_VOLUME_MULTIPLIER;
        case CreatureSoundEvent::SWIMMING:
        case CreatureSoundEvent::FLYING:
            return 0.2f * m_creatureVolume;
        case CreatureSoundEvent::ALERT:
            return 0.5f * m_creatureVolume;  // Moderate
        case CreatureSoundEvent::MATING_CALL:
            return 0.45f * m_creatureVolume;
        case CreatureSoundEvent::HUNTING:
            return 0.35f * m_creatureVolume;
        case CreatureSoundEvent::PAIN:
            return 0.4f * m_creatureVolume;
        case CreatureSoundEvent::DEATH:
            return 0.35f * m_creatureVolume;
        case CreatureSoundEvent::BIRTH:
            return 0.3f * m_creatureVolume;
        case CreatureSoundEvent::ATTACKING:
            return 0.45f * m_creatureVolume;
        default:
            return 0.3f * m_creatureVolume;
    }
}

inline float CreatureVoiceGenerator::getImportanceForEvent(CreatureSoundEvent event) const {
    switch (event) {
        case CreatureSoundEvent::IDLE:
            return SoundImportance::IDLE;
        case CreatureSoundEvent::EATING:
            return SoundImportance::EATING;
        case CreatureSoundEvent::ALERT:
            return SoundImportance::ALERT;
        case CreatureSoundEvent::MATING_CALL:
            return SoundImportance::MATING;
        case CreatureSoundEvent::PAIN:
        case CreatureSoundEvent::ATTACKING:
            return SoundImportance::BEING_ATTACKED;
        case CreatureSoundEvent::HUNTING:
            return SoundImportance::HUNTING;
        case CreatureSoundEvent::DEATH:
            return SoundImportance::DEATH;
        case CreatureSoundEvent::BIRTH:
            return SoundImportance::BIRTH;
        default:
            return SoundImportance::MOVING;
    }
}

} // namespace Forge
