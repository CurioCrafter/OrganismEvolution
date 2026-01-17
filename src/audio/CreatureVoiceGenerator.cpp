#include "CreatureVoiceGenerator.h"
#include <random>
#include <ctime>

namespace Forge {

// ============================================================================
// Constructor
// ============================================================================

CreatureVoiceGenerator::CreatureVoiceGenerator() {
    std::random_device rd;
    m_rng.seed(rd());
}

// ============================================================================
// Initialization
// ============================================================================

void CreatureVoiceGenerator::init(AudioManager* audio, SoundscapeBudget* budget) {
    m_audio = audio;
    m_budget = budget;
}

// ============================================================================
// Vocalization API
// ============================================================================

bool CreatureVoiceGenerator::tryPlaySound(Creature& creature, CreatureSoundEvent event) {
    if (!m_enabled || !m_audio || !creature.isAlive()) return false;

    auto& state = getState(creature);

    // Check cooldown
    if (!checkCooldown(state, event)) return false;

    // For idle sounds, add probability check
    if (event == CreatureSoundEvent::IDLE) {
        if (m_randomDist(m_rng) > VocalizationCooldowns::IDLE_PROBABILITY) {
            return false;
        }
    }

    // Generate sound
    auto buffer = generateSoundForEvent(creature, event);
    if (buffer.empty()) return false;

    // Try to play
    return playSound(creature, state, event, buffer);
}

bool CreatureVoiceGenerator::forcePlaySound(Creature& creature, CreatureSoundEvent event) {
    if (!m_enabled || !m_audio || !creature.isAlive()) return false;

    auto& state = getState(creature);

    // Generate sound
    auto buffer = generateSoundForEvent(creature, event);
    if (buffer.empty()) return false;

    // Force play (bypass cooldown)
    return playSound(creature, state, event, buffer);
}

// ============================================================================
// Update
// ============================================================================

void CreatureVoiceGenerator::update(float deltaTime,
                                    const std::vector<std::unique_ptr<Creature>>& creatures) {
    if (!m_enabled) return;

    // Update all creature states
    updateCooldowns(deltaTime);

    // Trigger ambient vocalizations for living creatures
    for (const auto& creature : creatures) {
        if (!creature || !creature->isAlive()) continue;

        // Try idle sounds occasionally
        tryPlaySound(*creature, CreatureSoundEvent::IDLE);

        // Flying creatures make wing sounds
        if (isFlying(creature->getType())) {
            tryPlaySound(*creature, CreatureSoundEvent::FLYING);
        }

        // Aquatic creatures make swimming sounds
        if (isAquatic(creature->getType())) {
            tryPlaySound(*creature, CreatureSoundEvent::SWIMMING);
        }
    }

    // Clean up states for dead creatures
    std::vector<int> toRemove;
    for (auto& [id, state] : m_creatureStates) {
        bool found = false;
        for (const auto& creature : creatures) {
            if (creature && creature->getID() == id && creature->isAlive()) {
                found = true;
                break;
            }
        }
        if (!found) {
            toRemove.push_back(id);
        }
    }

    for (int id : toRemove) {
        m_creatureStates.erase(id);
    }
}

void CreatureVoiceGenerator::updateCooldowns(float deltaTime) {
    for (auto& [id, state] : m_creatureStates) {
        if (state.idleCooldown > 0.0f) state.idleCooldown -= deltaTime;
        if (state.alertCooldown > 0.0f) state.alertCooldown -= deltaTime;
        if (state.matingCooldown > 0.0f) state.matingCooldown -= deltaTime;
        if (state.eatingCooldown > 0.0f) state.eatingCooldown -= deltaTime;
        if (state.painCooldown > 0.0f) state.painCooldown -= deltaTime;
        if (state.movementCooldown > 0.0f) state.movementCooldown -= deltaTime;
    }
}

// ============================================================================
// Internal Methods
// ============================================================================

CreatureVocalizationState& CreatureVoiceGenerator::getState(Creature& creature) {
    int id = creature.getID();

    auto it = m_creatureStates.find(id);
    if (it != m_creatureStates.end()) {
        return it->second;
    }

    // Create new state from genome
    CreatureVocalizationState state;
    const Genome& genome = creature.getGenome();

    state.basePitch = getBasePitchFromGenome(genome);
    state.tempo = getTempoFromGenome(genome);
    state.voiceCharacter = getVoiceTypeFromCreatureType(creature.getType());

    // Set initial random cooldowns to stagger sounds
    state.idleCooldown = m_randomDist(m_rng) * VocalizationCooldowns::IDLE_MAX;

    m_creatureStates[id] = state;
    return m_creatureStates[id];
}

std::vector<int16_t> CreatureVoiceGenerator::generateSoundForEvent(Creature& creature,
                                                                    CreatureSoundEvent event) {
    const Genome& genome = creature.getGenome();
    CreatureType type = creature.getType();

    SynthParams params;

    switch (event) {
        case CreatureSoundEvent::IDLE: {
            if (isHerbivore(type)) {
                params = m_synthesizer.createHerbivoreCoo(genome.size);
            } else if (isPredator(type)) {
                params = m_synthesizer.createCarnivoreGrowl(genome.size);
                params.volume *= 0.5f;  // Quieter for idle
            } else if (isBirdType(type)) {
                params = m_synthesizer.createBirdChirp(genome.wingSpan);
            } else if (isInsectType(type)) {
                params = m_synthesizer.createInsectBuzz(genome.flapFrequency);
            } else if (isAquatic(type)) {
                params = m_synthesizer.createFishBubble(genome.size);
            } else {
                params = m_synthesizer.createHerbivoreCoo(genome.size);
            }
            break;
        }

        case CreatureSoundEvent::ALERT: {
            params = m_synthesizer.createAlarmCall(genome.size);
            break;
        }

        case CreatureSoundEvent::MATING_CALL: {
            params = m_synthesizer.createMatingCall(genome.size, isBirdType(type));
            break;
        }

        case CreatureSoundEvent::EATING: {
            params = m_synthesizer.createGrazingSound();
            break;
        }

        case CreatureSoundEvent::PAIN:
        case CreatureSoundEvent::DEATH: {
            params = m_synthesizer.createPainSound(genome.size);
            break;
        }

        case CreatureSoundEvent::BIRTH: {
            // Use a soft version of the species' idle sound
            if (isHerbivore(type)) {
                params = m_synthesizer.createHerbivoreCoo(genome.size * 0.7f);  // Higher pitch
            } else if (isBirdType(type)) {
                params = m_synthesizer.createBirdChirp(genome.wingSpan * 0.8f);
            } else {
                params = m_synthesizer.createHerbivoreCoo(genome.size * 0.7f);
            }
            params.volume *= 0.6f;
            break;
        }

        case CreatureSoundEvent::HUNTING:
        case CreatureSoundEvent::ATTACKING: {
            params = m_synthesizer.createCarnivoreHunt(genome.size);
            break;
        }

        case CreatureSoundEvent::SWIMMING: {
            params = m_synthesizer.createFishBubble(genome.size);
            params.volume *= 0.3f;  // Very subtle
            break;
        }

        case CreatureSoundEvent::FLYING: {
            if (isInsectType(type)) {
                params = m_synthesizer.createInsectBuzz(genome.flapFrequency);
            } else {
                // Wing flap sounds for birds
                SynthParams flapParams;
                flapParams.voiceType = VoiceType::NOISE_FILTERED;
                flapParams.duration = 0.1f;
                flapParams.volume = 0.15f;
                flapParams.envelope = Envelope::percussive();
                flapParams.filterCutoff = 1500.0f;
                flapParams.filterResonance = 0.3f;
                params = flapParams;
            }
            break;
        }

        default:
            params = m_synthesizer.createHerbivoreCoo(genome.size);
            break;
    }

    // Apply event volume
    params.volume = getVolumeForEvent(event);

    return m_synthesizer.generate(params);
}

bool CreatureVoiceGenerator::checkCooldown(CreatureVocalizationState& state,
                                           CreatureSoundEvent event) {
    switch (event) {
        case CreatureSoundEvent::IDLE:
            return state.idleCooldown <= 0.0f;
        case CreatureSoundEvent::ALERT:
            return state.alertCooldown <= 0.0f;
        case CreatureSoundEvent::MATING_CALL:
            return state.matingCooldown <= 0.0f;
        case CreatureSoundEvent::EATING:
            return state.eatingCooldown <= 0.0f;
        case CreatureSoundEvent::PAIN:
            return state.painCooldown <= 0.0f;
        case CreatureSoundEvent::DEATH:
            return true;  // Death sounds always play
        case CreatureSoundEvent::SWIMMING:
        case CreatureSoundEvent::FLYING:
            return state.movementCooldown <= 0.0f;
        case CreatureSoundEvent::ATTACKING:
            return state.painCooldown <= 0.0f;  // Reuse pain cooldown
        default:
            return true;
    }
}

void CreatureVoiceGenerator::resetCooldown(CreatureVocalizationState& state,
                                            CreatureSoundEvent event) {
    switch (event) {
        case CreatureSoundEvent::IDLE:
            // Random cooldown between min and max
            state.idleCooldown = VocalizationCooldowns::IDLE_MIN +
                m_randomDist(m_rng) * (VocalizationCooldowns::IDLE_MAX - VocalizationCooldowns::IDLE_MIN);
            break;
        case CreatureSoundEvent::ALERT:
            state.alertCooldown = VocalizationCooldowns::ALERT;
            break;
        case CreatureSoundEvent::MATING_CALL:
            state.matingCooldown = VocalizationCooldowns::MATING;
            break;
        case CreatureSoundEvent::EATING:
            state.eatingCooldown = VocalizationCooldowns::EATING;
            break;
        case CreatureSoundEvent::PAIN:
        case CreatureSoundEvent::ATTACKING:
            state.painCooldown = VocalizationCooldowns::PAIN;
            break;
        case CreatureSoundEvent::SWIMMING:
        case CreatureSoundEvent::FLYING:
            state.movementCooldown = VocalizationCooldowns::MOVEMENT;
            break;
        default:
            break;
    }
}

bool CreatureVoiceGenerator::playSound(Creature& creature, CreatureVocalizationState& state,
                                       CreatureSoundEvent event,
                                       const std::vector<int16_t>& buffer) {
    if (buffer.empty()) return false;

    // Check vocalization budget (prevents too many nearby creatures vocalizing)
    if (m_budget) {
        float delay = m_budget->getVocalizationDelay(creature.getPosition());
        if (delay > 0.0f) {
            // Would need to queue - for simplicity, we just skip
            return false;
        }
    }

    // Create sound request
    SoundRequest request;
    request.category = SoundCategory::CREATURES;
    request.position = creature.getPosition();
    request.volume = getVolumeForEvent(event);
    request.importance = getImportanceForEvent(event);
    request.creatureId = creature.getID();
    request.customBuffer = buffer;
    request.useCustomBuffer = true;
    request.duration = buffer.size() / (44100.0f * 2.0f);  // Stereo 44.1kHz

    // Try to play through budget system
    if (m_budget) {
        bool played = m_budget->requestSound(request);
        if (played) {
            resetCooldown(state, event);
            m_budget->registerVocalization(creature.getPosition(), request.duration);
        }
        return played;
    }

    // Direct playback if no budget system
    if (m_audio) {
        state.lastSound = m_audio->playBuffer(buffer, creature.getPosition(), request.volume);
        if (state.lastSound.valid) {
            resetCooldown(state, event);
            return true;
        }
    }

    return false;
}

} // namespace Forge
