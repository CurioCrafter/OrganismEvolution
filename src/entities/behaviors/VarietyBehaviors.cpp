#include "VarietyBehaviors.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include "../../utils/SpatialGrid.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// ============================================================================
// CreatureMemory Implementation
// ============================================================================

void CreatureMemory::update(float currentTime) {
    // Decay food memory
    if (hasValidFoodMemory && (currentTime - lastFoodTime) > FOOD_MEMORY_DECAY) {
        hasValidFoodMemory = false;
    }

    // Decay threat memory
    if (hasValidThreatMemory && (currentTime - lastThreatTime) > THREAT_MEMORY_DECAY) {
        hasValidThreatMemory = false;
    }

    // Decay mate memory
    if (hasMateMemory && (currentTime - lastMateTime) > MATE_MEMORY_DECAY) {
        hasMateMemory = false;
    }

    // Decay carcass memory
    if (hasCarcassMemory && (currentTime - lastCarcassTime) > CARCASS_MEMORY_DECAY) {
        hasCarcassMemory = false;
    }

    // Prune old location memories (keep only recent)
    while (recentLocations.size() > MAX_RECENT_LOCATIONS) {
        recentLocations.erase(recentLocations.begin());
    }
}

void CreatureMemory::rememberFood(const glm::vec3& pos, float time) {
    lastFoodLocation = pos;
    lastFoodTime = time;
    hasValidFoodMemory = true;
}

void CreatureMemory::rememberThreat(const glm::vec3& pos, uint32_t threatId, float time) {
    lastThreatLocation = pos;
    lastThreatId = threatId;
    lastThreatTime = time;
    hasValidThreatMemory = true;
}

void CreatureMemory::rememberMate(const glm::vec3& pos, uint32_t mateId, float time) {
    lastMateLocation = pos;
    lastMateId = mateId;
    lastMateTime = time;
    hasMateMemory = true;
}

void CreatureMemory::rememberCarcass(const glm::vec3& pos, float time) {
    lastCarcassLocation = pos;
    lastCarcassTime = time;
    hasCarcassMemory = true;
}

void CreatureMemory::addVisitedLocation(const glm::vec3& pos) {
    // Only add if sufficiently far from recent locations
    for (const auto& loc : recentLocations) {
        if (glm::distance(loc, pos) < 5.0f) {
            return;  // Too close to existing memory
        }
    }
    recentLocations.push_back(pos);
}

float CreatureMemory::getNoveltyScore(const glm::vec3& pos) const {
    if (recentLocations.empty()) {
        return 1.0f;  // Everything is novel
    }

    float minDist = FLT_MAX;
    for (const auto& loc : recentLocations) {
        float dist = glm::distance(loc, pos);
        minDist = std::min(minDist, dist);
    }

    // Novelty increases with distance from known locations
    return std::min(1.0f, minDist / 50.0f);
}

void CreatureMemory::clear() {
    hasValidFoodMemory = false;
    hasValidThreatMemory = false;
    hasMateMemory = false;
    hasCarcassMemory = false;
    recentLocations.clear();
}

// ============================================================================
// BehaviorPersonality Implementation
// ============================================================================

void BehaviorPersonality::initFromGenome(float genomeAggression, float genomeSize, float genomeSpeed) {
    // Derive personality from genome traits
    aggression = genomeAggression;

    // Larger creatures tend to be bolder
    boldness = std::clamp(0.3f + genomeSize * 0.5f, 0.0f, 1.0f);

    // Faster creatures tend to be more curious (can escape if needed)
    curiosity = std::clamp(0.4f + genomeSpeed * 0.3f, 0.0f, 1.0f);

    // Size inversely correlates with sociability for predators
    sociability = std::clamp(0.6f - genomeAggression * 0.3f, 0.0f, 1.0f);

    // Patience varies with aggression
    patience = std::clamp(0.7f - genomeAggression * 0.4f, 0.0f, 1.0f);
}

void BehaviorPersonality::addRandomVariation(uint32_t seed) {
    // Simple hash-based variation
    auto hash = [](uint32_t x) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    };

    auto randFloat = [&](uint32_t s) {
        return (hash(s) % 1000) / 1000.0f;
    };

    // Small variations (+/- 0.1)
    curiosity = std::clamp(curiosity + (randFloat(seed) - 0.5f) * 0.2f, 0.0f, 1.0f);
    aggression = std::clamp(aggression + (randFloat(seed + 1) - 0.5f) * 0.2f, 0.0f, 1.0f);
    sociability = std::clamp(sociability + (randFloat(seed + 2) - 0.5f) * 0.2f, 0.0f, 1.0f);
    boldness = std::clamp(boldness + (randFloat(seed + 3) - 0.5f) * 0.2f, 0.0f, 1.0f);
    patience = std::clamp(patience + (randFloat(seed + 4) - 0.5f) * 0.2f, 0.0f, 1.0f);
}

// ============================================================================
// BehaviorState Implementation
// ============================================================================

std::string BehaviorState::getStateName() const {
    switch (currentBehavior) {
        case VarietyBehaviorType::NONE: return "None";
        case VarietyBehaviorType::IDLE: return "Idle";
        case VarietyBehaviorType::WANDERING: return "Wandering";
        case VarietyBehaviorType::CURIOSITY_APPROACH: return "Curious (Approaching)";
        case VarietyBehaviorType::CURIOSITY_INSPECT: return "Curious (Inspecting)";
        case VarietyBehaviorType::MATING_DISPLAY: return "Mating Display";
        case VarietyBehaviorType::MATING_APPROACH: return "Approaching Mate";
        case VarietyBehaviorType::SCAVENGING_SEEK: return "Seeking Carcass";
        case VarietyBehaviorType::SCAVENGING_FEED: return "Scavenging";
        case VarietyBehaviorType::RESTING: return "Resting";
        case VarietyBehaviorType::GROOMING: return "Grooming";
        case VarietyBehaviorType::PLAYING: return "Playing";
        default: return "Unknown";
    }
}

// ============================================================================
// VarietyBehaviorManager Implementation
// ============================================================================

VarietyBehaviorManager::VarietyBehaviorManager() {}

void VarietyBehaviorManager::init(CreatureManager* creatureManager, SpatialGrid* spatialGrid) {
    m_creatureManager = creatureManager;
    m_spatialGrid = spatialGrid;
}

void VarietyBehaviorManager::reset() {
    m_creatureData.clear();
    m_carcasses.clear();
    m_stats = Stats();
}

void VarietyBehaviorManager::registerCreature(uint32_t creatureId, const BehaviorPersonality& personality) {
    CreatureData data;
    data.personality = personality;
    data.state.currentBehavior = VarietyBehaviorType::WANDERING;
    m_creatureData[creatureId] = data;
}

void VarietyBehaviorManager::unregisterCreature(uint32_t creatureId) {
    m_creatureData.erase(creatureId);
}

void VarietyBehaviorManager::update(float deltaTime, float currentTime) {
    // Update carcasses
    updateCarcasses(deltaTime);

    // Update each creature's memory
    for (auto& [id, data] : m_creatureData) {
        data.memory.update(currentTime);
    }
}

glm::vec3 VarietyBehaviorManager::calculateBehaviorForce(Creature* creature, float currentTime) {
    if (!creature || !creature->isAlive()) {
        return glm::vec3(0.0f);
    }

    uint32_t id = creature->getId();
    auto it = m_creatureData.find(id);
    if (it == m_creatureData.end()) {
        // Auto-register with default personality
        BehaviorPersonality personality;
        const Genome& genome = creature->getGenome();
        float aggressionSeed = isPredator(creature->getType()) ? 0.7f : 0.3f;
        personality.initFromGenome(aggressionSeed, genome.size, genome.speed);
        personality.addRandomVariation(id);
        registerCreature(id, personality);
        it = m_creatureData.find(id);
    }

    CreatureData& data = it->second;

    // Update location memory periodically
    data.memory.addVisitedLocation(creature->getPosition());

    // Select behavior if none active or transition requested
    if (data.state.currentBehavior == VarietyBehaviorType::NONE ||
        data.state.transitionRequested) {
        selectBehavior(creature, data, currentTime);
    }

    // Check for behavior completion/timeout
    float behaviorElapsed = currentTime - data.state.behaviorStartTime;
    if (behaviorElapsed > data.state.behaviorDuration && data.state.behaviorDuration > 0.0f) {
        transitionBehavior(data, VarietyBehaviorType::WANDERING, currentTime);
    }

    // Calculate force based on current behavior
    glm::vec3 force(0.0f);

    switch (data.state.currentBehavior) {
        case VarietyBehaviorType::CURIOSITY_APPROACH:
        case VarietyBehaviorType::CURIOSITY_INSPECT:
            force = calculateCuriosityForce(creature, data, currentTime);
            break;

        case VarietyBehaviorType::MATING_DISPLAY:
        case VarietyBehaviorType::MATING_APPROACH:
            force = calculateMatingDisplayForce(creature, data, currentTime);
            break;

        case VarietyBehaviorType::SCAVENGING_SEEK:
        case VarietyBehaviorType::SCAVENGING_FEED:
            force = calculateScavengingForce(creature, data, currentTime);
            break;

        case VarietyBehaviorType::PLAYING:
            force = calculatePlayForce(creature, data, currentTime);
            break;

        case VarietyBehaviorType::RESTING:
        case VarietyBehaviorType::GROOMING:
            force = calculateRestingForce(creature, data, currentTime);
            break;

        default:
            break;
    }

    return force;
}

glm::vec3 VarietyBehaviorManager::calculateCuriosityForce(Creature* creature, CreatureData& data, float currentTime) {
    glm::vec3 pos = creature->getPosition();
    glm::vec3 target = data.state.targetPosition;
    glm::vec3 direction = target - pos;
    float dist = glm::length(direction);

    if (dist < 0.1f) {
        // Arrived at target - transition to inspect
        if (data.state.currentBehavior == VarietyBehaviorType::CURIOSITY_APPROACH) {
            transitionBehavior(data, VarietyBehaviorType::CURIOSITY_INSPECT, currentTime);
            return glm::vec3(0.0f);
        }
        // Inspection complete
        data.state.transitionRequested = true;
        return glm::vec3(0.0f);
    }

    // Approach force - slower approach for curiosity (cautious)
    float approachSpeed = 0.4f * data.personality.boldness;

    if (data.state.currentBehavior == VarietyBehaviorType::CURIOSITY_INSPECT) {
        // Circle around target while inspecting
        float angle = currentTime * 0.5f;
        glm::vec3 orbitOffset(std::cos(angle) * 3.0f, 0.0f, std::sin(angle) * 3.0f);
        target = data.state.targetPosition + orbitOffset;
        direction = target - pos;
        dist = glm::length(direction);
        approachSpeed = 0.6f;
    }

    if (dist > 0.1f) {
        return glm::normalize(direction) * approachSpeed;
    }

    return glm::vec3(0.0f);
}

glm::vec3 VarietyBehaviorManager::calculateMatingDisplayForce(Creature* creature, CreatureData& data, float currentTime) {
    glm::vec3 pos = creature->getPosition();

    if (data.state.currentBehavior == VarietyBehaviorType::MATING_DISPLAY) {
        // During display, stay relatively still with small movements
        float displayPhase = (currentTime - data.state.behaviorStartTime) * 2.0f;

        // Small circular movement pattern (display dance)
        float displayRadius = 1.5f;
        glm::vec3 displayOffset(
            std::sin(displayPhase) * displayRadius,
            std::sin(displayPhase * 2.0f) * 0.3f,  // Small bobbing
            std::cos(displayPhase) * displayRadius
        );

        // Energy cost for display
        data.state.displayEnergyCost += 0.5f * (currentTime - data.state.behaviorStartTime);
        data.state.displayProgress = std::min(1.0f, (currentTime - data.state.behaviorStartTime) / 5.0f);

        // After display completes, approach mate
        if (data.state.displayProgress >= 1.0f && data.state.targetCreatureId != 0) {
            transitionBehavior(data, VarietyBehaviorType::MATING_APPROACH, currentTime);
        }

        return displayOffset * 0.3f;
    }

    // Mating approach
    if (data.state.currentBehavior == VarietyBehaviorType::MATING_APPROACH) {
        glm::vec3 direction = data.state.targetPosition - pos;
        float dist = glm::length(direction);

        if (dist < 2.0f) {
            // Close enough - let reproduction logic handle it
            data.state.transitionRequested = true;
            return glm::vec3(0.0f);
        }

        // Eager approach to mate
        return glm::normalize(direction) * 0.8f;
    }

    return glm::vec3(0.0f);
}

glm::vec3 VarietyBehaviorManager::calculateScavengingForce(Creature* creature, CreatureData& data, float currentTime) {
    glm::vec3 pos = creature->getPosition();

    if (data.state.currentBehavior == VarietyBehaviorType::SCAVENGING_SEEK) {
        // Move toward remembered or detected carcass
        glm::vec3 target = data.memory.hasCarcassMemory ?
                          data.memory.lastCarcassLocation : data.state.targetPosition;

        glm::vec3 direction = target - pos;
        float dist = glm::length(direction);

        if (dist < 3.0f) {
            // Arrived at carcass - start feeding
            transitionBehavior(data, VarietyBehaviorType::SCAVENGING_FEED, currentTime);
            return glm::vec3(0.0f);
        }

        // Hungry approach - move faster based on hunger
        float hungerFactor = 1.0f - (creature->getEnergy() / creature->getMaxEnergy());
        return glm::normalize(direction) * (0.5f + hungerFactor * 0.4f);
    }

    if (data.state.currentBehavior == VarietyBehaviorType::SCAVENGING_FEED) {
        // Stay at carcass while feeding
        // Slight jittery movement while eating
        float feedPhase = currentTime * 3.0f;
        return glm::vec3(
            std::sin(feedPhase) * 0.1f,
            0.0f,
            std::cos(feedPhase * 1.3f) * 0.1f
        );
    }

    return glm::vec3(0.0f);
}

glm::vec3 VarietyBehaviorManager::calculatePlayForce(Creature* creature, CreatureData& data, float currentTime) {
    glm::vec3 pos = creature->getPosition();

    // Play involves quick, unpredictable movements
    float playPhase = (currentTime - data.state.behaviorStartTime) * 4.0f;

    // Figure-eight pattern with some randomness
    float radius = 5.0f;
    glm::vec3 playTarget(
        std::sin(playPhase) * radius,
        std::abs(std::sin(playPhase * 0.5f)) * 2.0f,  // Occasional jumps
        std::sin(playPhase * 2.0f) * radius * 0.5f
    );

    glm::vec3 direction = playTarget;
    float mag = glm::length(direction);
    if (mag > 0.1f) {
        return glm::normalize(direction) * 0.7f;
    }

    return glm::vec3(0.0f);
}

glm::vec3 VarietyBehaviorManager::calculateRestingForce(Creature* creature, CreatureData& data, float currentTime) {
    // Resting: minimal movement, energy conservation
    // Small random shifts to avoid looking frozen
    float restPhase = currentTime * 0.3f;
    return glm::vec3(
        std::sin(restPhase) * 0.02f,
        0.0f,
        std::cos(restPhase * 0.7f) * 0.02f
    );
}

std::vector<BehaviorPriority> VarietyBehaviorManager::evaluateBehaviors(Creature* creature, CreatureData& data, float currentTime) {
    std::vector<BehaviorPriority> priorities;

    float energy = creature->getEnergy();
    float maxEnergy = creature->getMaxEnergy();
    float energyRatio = energy / maxEnergy;
    float age = creature->getAge();
    const auto& personality = data.personality;

    // Curiosity - triggered by novelty and personality
    glm::vec3 novelPos;
    if (detectNovelStimulus(creature, data, novelPos)) {
        float curiosityUrge = personality.curiosity * data.memory.getNoveltyScore(novelPos);
        if (energyRatio > 0.4f) {  // Only curious when not too hungry
            priorities.push_back({VarietyBehaviorType::CURIOSITY_APPROACH, PRIORITY_CURIOSITY, curiosityUrge});
        }
    }

    // Mating display - triggered by reproductive readiness and mate presence
    glm::vec3 matePos;
    uint32_t mateId;
    if (creature->canReproduce() && detectPotentialMate(creature, data, matePos, mateId)) {
        float matingUrge = 0.5f + personality.boldness * 0.3f;
        priorities.push_back({VarietyBehaviorType::MATING_DISPLAY, PRIORITY_MATING, matingUrge});
    }

    // Scavenging - for opportunistic feeders when hungry
    glm::vec3 carcassPos;
    if (energyRatio < 0.6f && detectCarcassNearby(creature, data, carcassPos)) {
        float scavengeUrge = (1.0f - energyRatio) * (1.0f - personality.aggression * 0.5f);
        priorities.push_back({VarietyBehaviorType::SCAVENGING_SEEK, PRIORITY_HUNGER, scavengeUrge});
    }

    // Playing - juveniles and social creatures
    if (age < 30.0f && personality.sociability > 0.5f && energyRatio > 0.6f) {
        float playUrge = personality.sociability * (1.0f - age / 60.0f);
        priorities.push_back({VarietyBehaviorType::PLAYING, PRIORITY_SOCIAL, playUrge});
    }

    // Resting - low energy or after exertion
    if (energyRatio < 0.3f) {
        float restUrge = (1.0f - energyRatio);
        priorities.push_back({VarietyBehaviorType::RESTING, PRIORITY_SURVIVAL * 0.5f, restUrge});
    }

    // Default wandering
    priorities.push_back({VarietyBehaviorType::WANDERING, PRIORITY_IDLE, 0.5f});

    // Sort by priority * urgency
    std::sort(priorities.begin(), priorities.end());

    return priorities;
}

void VarietyBehaviorManager::selectBehavior(Creature* creature, CreatureData& data, float currentTime) {
    auto priorities = evaluateBehaviors(creature, data, currentTime);

    data.state.transitionRequested = false;

    for (const auto& priority : priorities) {
        if (canTransitionTo(priority.behavior, data, currentTime)) {
            transitionBehavior(data, priority.behavior, currentTime);

            // Set up behavior-specific data
            switch (priority.behavior) {
                case VarietyBehaviorType::CURIOSITY_APPROACH:
                case VarietyBehaviorType::CURIOSITY_INSPECT: {
                    glm::vec3 novelPos;
                    if (detectNovelStimulus(creature, data, novelPos)) {
                        data.state.targetPosition = novelPos;
                        data.state.behaviorDuration = 8.0f + data.personality.curiosity * 4.0f;
                    }
                    m_stats.curiosityBehaviors++;
                    break;
                }

                case VarietyBehaviorType::MATING_DISPLAY: {
                    glm::vec3 matePos;
                    uint32_t mateId;
                    if (detectPotentialMate(creature, data, matePos, mateId)) {
                        data.state.targetPosition = matePos;
                        data.state.targetCreatureId = mateId;
                        data.state.behaviorDuration = 5.0f + data.personality.patience * 3.0f;
                        data.state.displayProgress = 0.0f;
                    }
                    m_stats.matingDisplays++;
                    break;
                }

                case VarietyBehaviorType::SCAVENGING_SEEK: {
                    glm::vec3 carcassPos;
                    if (detectCarcassNearby(creature, data, carcassPos)) {
                        data.state.targetPosition = carcassPos;
                        data.state.behaviorDuration = 15.0f;
                    }
                    m_stats.scavengingBehaviors++;
                    break;
                }

                case VarietyBehaviorType::PLAYING: {
                    data.state.behaviorDuration = 10.0f + data.personality.sociability * 5.0f;
                    m_stats.playBehaviors++;
                    break;
                }

                case VarietyBehaviorType::RESTING: {
                    data.state.behaviorDuration = 5.0f + (1.0f - creature->getEnergy() / creature->getMaxEnergy()) * 10.0f;
                    break;
                }

                default:
                    data.state.behaviorDuration = 5.0f;
                    break;
            }

            return;
        }
    }
}

bool VarietyBehaviorManager::canTransitionTo(VarietyBehaviorType newBehavior, const CreatureData& data, float currentTime) {
    // Check cooldown
    if (currentTime < data.state.cooldownEndTime) {
        return false;
    }

    // Specific transition rules
    switch (newBehavior) {
        case VarietyBehaviorType::CURIOSITY_APPROACH:
            return (currentTime - data.state.cooldownEndTime) > CURIOSITY_COOLDOWN ||
                   data.state.currentBehavior == VarietyBehaviorType::WANDERING;

        case VarietyBehaviorType::MATING_DISPLAY:
            return true;  // Always allow mating if conditions met

        case VarietyBehaviorType::SCAVENGING_SEEK:
            return true;

        case VarietyBehaviorType::PLAYING:
            return (currentTime - data.state.cooldownEndTime) > PLAY_COOLDOWN;

        default:
            return true;
    }
}

void VarietyBehaviorManager::transitionBehavior(CreatureData& data, VarietyBehaviorType newBehavior, float currentTime) {
    VarietyBehaviorType oldBehavior = data.state.currentBehavior;

    data.state.previousBehavior = oldBehavior;
    data.state.currentBehavior = newBehavior;
    data.state.behaviorStartTime = currentTime;
    data.state.transitionRequested = false;

    // Set cooldown for previous behavior
    switch (oldBehavior) {
        case VarietyBehaviorType::CURIOSITY_APPROACH:
        case VarietyBehaviorType::CURIOSITY_INSPECT:
            data.state.cooldownEndTime = currentTime + CURIOSITY_COOLDOWN;
            break;
        case VarietyBehaviorType::MATING_DISPLAY:
        case VarietyBehaviorType::MATING_APPROACH:
            data.state.cooldownEndTime = currentTime + MATING_DISPLAY_COOLDOWN;
            break;
        case VarietyBehaviorType::SCAVENGING_SEEK:
        case VarietyBehaviorType::SCAVENGING_FEED:
            data.state.cooldownEndTime = currentTime + SCAVENGING_COOLDOWN;
            break;
        case VarietyBehaviorType::PLAYING:
            data.state.cooldownEndTime = currentTime + PLAY_COOLDOWN;
            break;
        default:
            break;
    }

    m_stats.totalTransitions++;

    if (m_debugLogging) {
        logBehaviorTransition(0, oldBehavior, newBehavior);
    }
}

bool VarietyBehaviorManager::detectNovelStimulus(Creature* creature, CreatureData& data, glm::vec3& outStimulusPos) {
    if (!m_spatialGrid) return false;

    glm::vec3 pos = creature->getPosition();
    float detectionRange = creature->getVisionRange() * 0.8f;

    // Query nearby creatures
    auto& nearby = m_spatialGrid->query(pos, detectionRange);

    float bestNovelty = 0.3f;  // Minimum novelty threshold
    glm::vec3 bestPos = pos;
    bool found = false;

    for (Creature* other : nearby) {
        if (!other || other == creature || !other->isAlive()) continue;

        glm::vec3 otherPos = other->getPosition();
        float novelty = data.memory.getNoveltyScore(otherPos);

        // Different species are more novel
        if (other->getSpeciesId() != creature->getSpeciesId()) {
            novelty *= 1.3f;
        }

        if (novelty > bestNovelty) {
            bestNovelty = novelty;
            bestPos = otherPos;
            found = true;
        }
    }

    if (found) {
        outStimulusPos = bestPos;
        return true;
    }

    return false;
}

bool VarietyBehaviorManager::detectPotentialMate(Creature* creature, CreatureData& data, glm::vec3& outMatePos, uint32_t& outMateId) {
    if (!m_spatialGrid) return false;

    glm::vec3 pos = creature->getPosition();
    float detectionRange = creature->getVisionRange();

    auto& nearby = m_spatialGrid->query(pos, detectionRange);

    for (Creature* other : nearby) {
        if (!other || other == creature || !other->isAlive()) continue;

        // Same species and can mate
        if (other->getSpeciesId() == creature->getSpeciesId() &&
            other->canReproduce() && creature->canMateWith(*other)) {
            outMatePos = other->getPosition();
            outMateId = other->getId();
            return true;
        }
    }

    return false;
}

bool VarietyBehaviorManager::detectCarcassNearby(Creature* creature, CreatureData& data, glm::vec3& outCarcassPos) {
    // Check memory first
    if (data.memory.hasCarcassMemory) {
        outCarcassPos = data.memory.lastCarcassLocation;
        return true;
    }

    // Check carcass list
    glm::vec3 pos = creature->getPosition();
    float detectionRange = creature->getVisionRange();

    for (const auto& carcass : m_carcasses) {
        if (!carcass.claimed && carcass.remainingFood > 0.0f) {
            float dist = glm::distance(pos, carcass.position);
            if (dist < detectionRange) {
                outCarcassPos = carcass.position;
                return true;
            }
        }
    }

    return false;
}

void VarietyBehaviorManager::updateCarcasses(float deltaTime) {
    // Decay carcasses over time
    for (auto it = m_carcasses.begin(); it != m_carcasses.end();) {
        it->remainingFood -= deltaTime * 0.5f;  // Decay rate
        if (it->remainingFood <= 0.0f) {
            it = m_carcasses.erase(it);
        } else {
            ++it;
        }
    }
}

void VarietyBehaviorManager::addCarcass(const glm::vec3& position, float time) {
    CarcassInfo carcass;
    carcass.position = position;
    carcass.spawnTime = time;
    carcass.remainingFood = 50.0f;  // Food units
    carcass.claimed = false;
    m_carcasses.push_back(carcass);
}

void VarietyBehaviorManager::onCreatureDeath(uint32_t creatureId, const glm::vec3& deathPos, float time) {
    // Create carcass at death location
    addCarcass(deathPos, time);

    // Notify nearby creatures
    if (m_spatialGrid) {
        auto& nearby = m_spatialGrid->query(deathPos, 50.0f);
        for (Creature* c : nearby) {
            if (c && c->isAlive()) {
                onCarcassFound(c->getId(), deathPos, time);
            }
        }
    }
}

void VarietyBehaviorManager::onFoodFound(uint32_t creatureId, const glm::vec3& foodPos, float time) {
    auto it = m_creatureData.find(creatureId);
    if (it != m_creatureData.end()) {
        it->second.memory.rememberFood(foodPos, time);
    }
}

void VarietyBehaviorManager::onThreatDetected(uint32_t creatureId, const glm::vec3& threatPos, uint32_t threatId, float time) {
    auto it = m_creatureData.find(creatureId);
    if (it != m_creatureData.end()) {
        it->second.memory.rememberThreat(threatPos, threatId, time);
    }
}

void VarietyBehaviorManager::onPotentialMateDetected(uint32_t creatureId, const glm::vec3& matePos, uint32_t mateId, float time) {
    auto it = m_creatureData.find(creatureId);
    if (it != m_creatureData.end()) {
        it->second.memory.rememberMate(matePos, mateId, time);
    }
}

void VarietyBehaviorManager::onCarcassFound(uint32_t creatureId, const glm::vec3& carcassPos, float time) {
    auto it = m_creatureData.find(creatureId);
    if (it != m_creatureData.end()) {
        it->second.memory.rememberCarcass(carcassPos, time);
    }
}

const BehaviorState* VarietyBehaviorManager::getBehaviorState(uint32_t creatureId) const {
    auto it = m_creatureData.find(creatureId);
    return it != m_creatureData.end() ? &it->second.state : nullptr;
}

const CreatureMemory* VarietyBehaviorManager::getMemory(uint32_t creatureId) const {
    auto it = m_creatureData.find(creatureId);
    return it != m_creatureData.end() ? &it->second.memory : nullptr;
}

std::string VarietyBehaviorManager::getBehaviorDescription(uint32_t creatureId) const {
    auto it = m_creatureData.find(creatureId);
    if (it == m_creatureData.end()) {
        return "Unknown";
    }
    return it->second.state.getStateName();
}

BehaviorPersonality* VarietyBehaviorManager::getPersonality(uint32_t creatureId) {
    auto it = m_creatureData.find(creatureId);
    return it != m_creatureData.end() ? &it->second.personality : nullptr;
}

const BehaviorPersonality* VarietyBehaviorManager::getPersonality(uint32_t creatureId) const {
    auto it = m_creatureData.find(creatureId);
    return it != m_creatureData.end() ? &it->second.personality : nullptr;
}

float VarietyBehaviorManager::calculateBehaviorWeight(VarietyBehaviorType behavior, const BehaviorPersonality& personality) {
    switch (behavior) {
        case VarietyBehaviorType::CURIOSITY_APPROACH:
        case VarietyBehaviorType::CURIOSITY_INSPECT:
            return personality.curiosity;

        case VarietyBehaviorType::MATING_DISPLAY:
        case VarietyBehaviorType::MATING_APPROACH:
            return personality.boldness * 0.7f + personality.patience * 0.3f;

        case VarietyBehaviorType::SCAVENGING_SEEK:
        case VarietyBehaviorType::SCAVENGING_FEED:
            return 1.0f - personality.aggression * 0.3f;  // Less aggressive creatures scavenge more

        case VarietyBehaviorType::PLAYING:
            return personality.sociability;

        case VarietyBehaviorType::RESTING:
            return 1.0f - personality.boldness * 0.5f;

        default:
            return 0.5f;
    }
}

void VarietyBehaviorManager::logBehaviorTransition(uint32_t creatureId, VarietyBehaviorType from, VarietyBehaviorType to) {
    BehaviorState tempFrom, tempTo;
    tempFrom.currentBehavior = from;
    tempTo.currentBehavior = to;
    std::cout << "[Behavior] Creature " << creatureId << ": "
              << tempFrom.getStateName() << " -> " << tempTo.getStateName() << std::endl;
}

// ============================================================================
// Aquatic Group Dynamics Implementation
// ============================================================================

namespace aquatic {

void SchoolDynamics::update(float deltaTime) {
    // Update panic wave propagation
    if (panicWaveRadius > 0.0f) {
        panicWaveRadius += panicWaveSpeed * deltaTime;

        // Decay wave after reaching certain size
        if (panicWaveRadius > 100.0f) {
            panicWaveRadius = 0.0f;
            if (state == SchoolBehaviorState::PANIC_SCATTER) {
                state = SchoolBehaviorState::REFORMING;
                stateStartTime = 0.0f;
                rejoinTimer = 5.0f;
            }
        }
    }

    // Update rejoin timer
    if (state == SchoolBehaviorState::REFORMING) {
        rejoinTimer -= deltaTime;
        if (rejoinTimer <= 0.0f) {
            state = SchoolBehaviorState::CRUISING;
            splitPositions.clear();
        }
    }

    // Update intensity based on state
    switch (state) {
        case SchoolBehaviorState::PANIC_SCATTER:
            intensityMultiplier = std::max(0.3f, intensityMultiplier - deltaTime * 0.5f);
            break;
        case SchoolBehaviorState::REFORMING:
            intensityMultiplier = std::min(1.0f, intensityMultiplier + deltaTime * 0.3f);
            break;
        case SchoolBehaviorState::FEEDING_FRENZY:
            intensityMultiplier = 0.7f;  // Looser formation while feeding
            break;
        case SchoolBehaviorState::LEADER_FOLLOWING:
            intensityMultiplier = 1.5f;  // Tighter formation following leader
            break;
        default:
            intensityMultiplier = 1.0f;
            break;
    }
}

void SchoolDynamics::triggerPanicWave(const glm::vec3& origin, float time) {
    panicOrigin = origin;
    panicWaveRadius = 1.0f;  // Start small
    state = SchoolBehaviorState::PANIC_SCATTER;
    stateStartTime = time;
}

void SchoolDynamics::requestSplit(int numGroups) {
    if (numGroups < 2) return;

    splitPositions.clear();

    // Generate split directions (radial)
    float angleStep = 6.28318f / numGroups;
    for (int i = 0; i < numGroups; ++i) {
        float angle = i * angleStep;
        splitPositions.push_back(glm::vec3(
            std::cos(angle) * splitDistance,
            0.0f,
            std::sin(angle) * splitDistance
        ));
    }
}

void SchoolDynamics::requestRejoin() {
    state = SchoolBehaviorState::REFORMING;
    rejoinTimer = 3.0f;
}

glm::vec3 calculateLeaderFollowForce(const glm::vec3& fishPos, const glm::vec3& leaderPos,
                                     const glm::vec3& leaderVel, float followDistance) {
    // Predict where leader will be
    glm::vec3 predictedPos = leaderPos + leaderVel * 0.5f;

    // Stay behind and to the side of leader
    glm::vec3 leaderDir = glm::length(leaderVel) > 0.01f ?
                          glm::normalize(leaderVel) : glm::vec3(1.0f, 0.0f, 0.0f);

    // Target position is behind leader at follow distance
    glm::vec3 targetPos = predictedPos - leaderDir * followDistance;

    glm::vec3 toTarget = targetPos - fishPos;
    float dist = glm::length(toTarget);

    if (dist < 0.5f) return glm::vec3(0.0f);

    // Stronger force when far from desired position
    float strength = std::min(dist / followDistance, 2.0f);
    return glm::normalize(toTarget) * strength;
}

glm::vec3 calculatePanicWaveForce(const glm::vec3& fishPos, const glm::vec3& panicOrigin,
                                  float waveRadius, float waveIntensity) {
    glm::vec3 awayDir = fishPos - panicOrigin;
    float dist = glm::length(awayDir);

    if (dist < 0.1f) {
        // At origin - flee in random direction
        return glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Wave front effect - strongest at wavefront
    float distFromWave = std::abs(dist - waveRadius);
    float waveFactor = std::exp(-distFromWave * 0.2f);

    // Only affect fish near the wavefront
    if (distFromWave > 10.0f) return glm::vec3(0.0f);

    return glm::normalize(awayDir) * waveFactor * waveIntensity;
}

float calculateLeaderScore(float age, float size, float energy, float survivalTime) {
    // Older, larger, healthier fish make better leaders
    float ageScore = std::min(age / 100.0f, 1.0f) * 0.3f;
    float sizeScore = std::min(size / 2.0f, 1.0f) * 0.3f;
    float energyScore = energy * 0.2f;
    float survivalScore = std::min(survivalTime / 300.0f, 1.0f) * 0.2f;

    return ageScore + sizeScore + energyScore + survivalScore;
}

} // namespace aquatic
