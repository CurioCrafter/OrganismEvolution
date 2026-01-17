#include "SensorySystem.h"
#include "Creature.h"
#include "CreatureType.h"
#include "../environment/Terrain.h"
#include "../utils/SpatialGrid.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>

// ============================================================================
// SensoryGenome Implementation
// ============================================================================

SensoryGenome::SensoryGenome() {
    // Default balanced sensory configuration
    visionFOV = 2.0f;            // ~115 degrees
    visionRange = 30.0f;
    visionAcuity = 0.5f;
    colorPerception = 0.3f;
    motionDetection = 0.5f;

    hearingRange = 40.0f;
    hearingDirectionality = 0.5f;
    echolocationAbility = 0.0f;

    smellRange = 50.0f;
    smellSensitivity = 0.5f;
    pheromoneProduction = 0.3f;

    touchRange = 2.0f;
    vibrationSensitivity = 0.3f;

    camouflageLevel = 0.0f;

    alarmCallVolume = 0.5f;
    displayIntensity = 0.3f;

    memoryCapacity = 0.5f;
    memoryRetention = 0.5f;
}

void SensoryGenome::randomize() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_real_distribution<float> fovDist(1.57f, 5.5f);  // π/2 to ~315 degrees
    std::uniform_real_distribution<float> rangeDist(15.0f, 50.0f);
    std::uniform_real_distribution<float> zeroOneDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> smellRangeDist(20.0f, 100.0f);
    std::uniform_real_distribution<float> hearingRangeDist(20.0f, 80.0f);
    std::uniform_real_distribution<float> touchRangeDist(1.0f, 5.0f);

    visionFOV = fovDist(gen);
    visionRange = rangeDist(gen);
    visionAcuity = zeroOneDist(gen);
    colorPerception = zeroOneDist(gen);
    motionDetection = zeroOneDist(gen);

    hearingRange = hearingRangeDist(gen);
    hearingDirectionality = zeroOneDist(gen);
    echolocationAbility = zeroOneDist(gen) * 0.3f;  // Rare trait

    smellRange = smellRangeDist(gen);
    smellSensitivity = zeroOneDist(gen);
    pheromoneProduction = zeroOneDist(gen);

    touchRange = touchRangeDist(gen);
    vibrationSensitivity = zeroOneDist(gen);

    camouflageLevel = zeroOneDist(gen) * 0.5f;  // Start with moderate camo at most

    alarmCallVolume = zeroOneDist(gen);
    displayIntensity = zeroOneDist(gen);

    memoryCapacity = zeroOneDist(gen);
    memoryRetention = zeroOneDist(gen);
}

void SensoryGenome::mutate(float mutationRate, float mutationStrength) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> chanceDist(0.0f, 1.0f);
    std::normal_distribution<float> mutationDist(0.0f, mutationStrength);

    auto mutateValue = [&](float& value, float minVal, float maxVal) {
        if (chanceDist(gen) < mutationRate) {
            value += mutationDist(gen) * (maxVal - minVal);
            value = std::clamp(value, minVal, maxVal);
        }
    };

    mutateValue(visionFOV, 1.0f, 6.0f);
    mutateValue(visionRange, 10.0f, 60.0f);
    mutateValue(visionAcuity, 0.0f, 1.0f);
    mutateValue(colorPerception, 0.0f, 1.0f);
    mutateValue(motionDetection, 0.0f, 1.0f);

    mutateValue(hearingRange, 10.0f, 100.0f);
    mutateValue(hearingDirectionality, 0.0f, 1.0f);
    mutateValue(echolocationAbility, 0.0f, 1.0f);

    mutateValue(smellRange, 10.0f, 150.0f);
    mutateValue(smellSensitivity, 0.0f, 1.0f);
    mutateValue(pheromoneProduction, 0.0f, 1.0f);

    mutateValue(touchRange, 0.5f, 8.0f);
    mutateValue(vibrationSensitivity, 0.0f, 1.0f);

    mutateValue(camouflageLevel, 0.0f, 1.0f);

    mutateValue(alarmCallVolume, 0.0f, 1.0f);
    mutateValue(displayIntensity, 0.0f, 1.0f);

    mutateValue(memoryCapacity, 0.0f, 1.0f);
    mutateValue(memoryRetention, 0.0f, 1.0f);
}

SensoryGenome SensoryGenome::crossover(const SensoryGenome& parent1, const SensoryGenome& parent2) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    SensoryGenome child;

    auto selectTrait = [&](float p1Val, float p2Val) {
        return dist(gen) < 0.5f ? p1Val : p2Val;
    };

    child.visionFOV = selectTrait(parent1.visionFOV, parent2.visionFOV);
    child.visionRange = selectTrait(parent1.visionRange, parent2.visionRange);
    child.visionAcuity = selectTrait(parent1.visionAcuity, parent2.visionAcuity);
    child.colorPerception = selectTrait(parent1.colorPerception, parent2.colorPerception);
    child.motionDetection = selectTrait(parent1.motionDetection, parent2.motionDetection);

    child.hearingRange = selectTrait(parent1.hearingRange, parent2.hearingRange);
    child.hearingDirectionality = selectTrait(parent1.hearingDirectionality, parent2.hearingDirectionality);
    child.echolocationAbility = selectTrait(parent1.echolocationAbility, parent2.echolocationAbility);

    child.smellRange = selectTrait(parent1.smellRange, parent2.smellRange);
    child.smellSensitivity = selectTrait(parent1.smellSensitivity, parent2.smellSensitivity);
    child.pheromoneProduction = selectTrait(parent1.pheromoneProduction, parent2.pheromoneProduction);

    child.touchRange = selectTrait(parent1.touchRange, parent2.touchRange);
    child.vibrationSensitivity = selectTrait(parent1.vibrationSensitivity, parent2.vibrationSensitivity);

    child.camouflageLevel = selectTrait(parent1.camouflageLevel, parent2.camouflageLevel);

    child.alarmCallVolume = selectTrait(parent1.alarmCallVolume, parent2.alarmCallVolume);
    child.displayIntensity = selectTrait(parent1.displayIntensity, parent2.displayIntensity);

    child.memoryCapacity = selectTrait(parent1.memoryCapacity, parent2.memoryCapacity);
    child.memoryRetention = selectTrait(parent1.memoryRetention, parent2.memoryRetention);

    return child;
}

float SensoryGenome::calculateEnergyCost() const {
    float cost = 0.0f;

    // Vision: highest cost, scales with quality
    cost += (visionFOV / 6.28f) * 0.08f;  // FOV cost
    cost += (visionRange / 60.0f) * 0.15f;  // Range cost
    cost += visionAcuity * 0.25f;  // Acuity is expensive
    cost += colorPerception * 0.15f;
    cost += motionDetection * 0.12f;

    // Hearing: moderate cost
    cost += (hearingRange / 100.0f) * 0.08f;
    cost += hearingDirectionality * 0.08f;
    cost += echolocationAbility * 0.35f;  // Echolocation is very expensive

    // Smell: low cost
    cost += (smellRange / 150.0f) * 0.04f;
    cost += smellSensitivity * 0.04f;
    cost += pheromoneProduction * 0.08f;

    // Touch: very low cost
    cost += (touchRange / 8.0f) * 0.02f;
    cost += vibrationSensitivity * 0.02f;

    // Camouflage: moderate cost (pigment production and behavior)
    cost += camouflageLevel * 0.12f;

    // Communication
    cost += alarmCallVolume * 0.05f;
    cost += displayIntensity * 0.08f;

    // Memory
    cost += memoryCapacity * 0.1f;
    cost += memoryRetention * 0.05f;

    return cost;
}

// ============================================================================
// SpatialMemory Implementation
// ============================================================================

SpatialMemory::SpatialMemory(int maxCapacity, float decayRate)
    : maxCapacity(maxCapacity), decayRate(decayRate), currentTime(0.0f) {}

void SpatialMemory::remember(const glm::vec3& position, MemoryType type, float importance) {
    // Check if we already have a memory near this location
    const float mergeDistance = 5.0f;
    for (auto& mem : memories) {
        if (mem.type == type && glm::length(mem.location - position) < mergeDistance) {
            // Reinforce existing memory
            mem.strength = std::min(1.0f, mem.strength + 0.3f);
            mem.timestamp = currentTime;
            mem.importance = std::max(mem.importance, importance);
            return;
        }
    }

    // Add new memory
    MemoryEntry entry(position, type, 1.0f, currentTime, importance);
    memories.push_back(entry);

    // Consolidate if over capacity
    if (static_cast<int>(memories.size()) > maxCapacity) {
        consolidate();
    }
}

void SpatialMemory::update(float deltaTime) {
    currentTime += deltaTime;

    // Decay all memories
    for (auto& mem : memories) {
        float decayFactor = decayRate * (1.0f - mem.importance * 0.5f);
        mem.strength -= decayFactor * deltaTime;
    }

    // Remove dead memories
    memories.erase(
        std::remove_if(memories.begin(), memories.end(),
            [](const MemoryEntry& m) { return m.strength <= 0.0f; }),
        memories.end()
    );
}

void SpatialMemory::clear() {
    memories.clear();
}

std::vector<MemoryEntry> SpatialMemory::recall(MemoryType type) const {
    std::vector<MemoryEntry> result;
    for (const auto& mem : memories) {
        if (mem.type == type && mem.strength > 0.1f) {
            result.push_back(mem);
        }
    }
    return result;
}

std::vector<MemoryEntry> SpatialMemory::recallNearby(const glm::vec3& position, float radius) const {
    std::vector<MemoryEntry> result;
    for (const auto& mem : memories) {
        if (glm::length(mem.location - position) < radius && mem.strength > 0.1f) {
            result.push_back(mem);
        }
    }
    return result;
}

bool SpatialMemory::hasMemoryOf(MemoryType type) const {
    for (const auto& mem : memories) {
        if (mem.type == type && mem.strength > 0.1f) {
            return true;
        }
    }
    return false;
}

glm::vec3 SpatialMemory::getClosestMemory(const glm::vec3& position, MemoryType type) const {
    float minDist = std::numeric_limits<float>::max();
    glm::vec3 closest = position;

    for (const auto& mem : memories) {
        if (mem.type == type && mem.strength > 0.1f) {
            float dist = glm::length(mem.location - position);
            if (dist < minDist) {
                minDist = dist;
                closest = mem.location;
            }
        }
    }

    return closest;
}

void SpatialMemory::consolidate() {
    // Sort by strength * importance
    std::sort(memories.begin(), memories.end(),
        [](const MemoryEntry& a, const MemoryEntry& b) {
            return (a.strength * a.importance) > (b.strength * b.importance);
        });

    // Keep only the strongest memories
    if (static_cast<int>(memories.size()) > maxCapacity) {
        memories.resize(maxCapacity);
    }
}

// ============================================================================
// SensorySystem Implementation
// ============================================================================

SensorySystem::SensorySystem()
    : genome(),
      memory(20, 0.1f) {}

SensorySystem::SensorySystem(const SensoryGenome& genome)
    : genome(genome),
      memory(static_cast<int>(10 + genome.memoryCapacity * 30),
             0.2f * (1.0f - genome.memoryRetention * 0.8f)) {}

void SensorySystem::sense(
    const glm::vec3& position,
    const glm::vec3& velocity,
    float facing,
    const std::vector<glm::vec3>& foodPositions,
    const std::vector<Creature*>& creatures,
    const SpatialGrid* spatialGrid,
    const Terrain& terrain,
    const EnvironmentConditions& environment,
    const std::vector<SoundEvent>& sounds,
    float currentTime
) {
    currentPercepts.clear();

    // Apply each sensory modality
    senseVision(position, facing, foodPositions, creatures, spatialGrid, environment, currentTime);
    senseHearing(position, sounds, creatures, environment, currentTime);
    senseSmell(position, foodPositions, creatures, environment, currentTime);
    senseTouch(position, creatures, environment, currentTime);

    // Sort percepts by distance for easy access
    std::sort(currentPercepts.begin(), currentPercepts.end(),
        [](const SensoryPercept& a, const SensoryPercept& b) {
            return a.distance < b.distance;
        });
}

void SensorySystem::senseVision(
    const glm::vec3& position,
    float facing,
    const std::vector<glm::vec3>& foodPositions,
    const std::vector<Creature*>& creatures,
    const SpatialGrid* spatialGrid,
    const EnvironmentConditions& environment,
    float currentTime
) {
    float effectiveRange = genome.visionRange * environment.visibility * environment.ambientLight;
    if (effectiveRange < 1.0f) return;  // Too dark to see

    float halfFOV = genome.visionFOV / 2.0f;

    // Detect food
    for (const auto& foodPos : foodPositions) {
        glm::vec3 toFood = foodPos - position;
        float distance = glm::length(toFood);

        if (distance > effectiveRange || distance < 0.1f) continue;

        float angle = atan2(toFood.z, toFood.x) - facing;
        angle = normalizeAngle(angle);

        if (!isInFieldOfView(angle, 0.0f, genome.visionFOV)) continue;

        float probability = calculateDetectionProbability(
            distance, effectiveRange, 0.0f, 0.0f, environment);

        if (probability > 0.1f) {
            SensoryPercept percept;
            percept.type = DetectionType::FOOD;
            percept.position = foodPos;
            percept.velocity = glm::vec3(0.0f);
            percept.distance = distance;
            percept.angle = angle;
            percept.confidence = probability;
            percept.strength = 1.0f - (distance / effectiveRange);
            percept.sensedBy = SensoryType::VISION;
            percept.sourceCreature = nullptr;
            percept.timestamp = currentTime;
            currentPercepts.push_back(percept);
        }
    }

    // Detect creatures
    for (Creature* other : creatures) {
        if (!other || !other->isAlive()) continue;

        glm::vec3 toOther = other->getPosition() - position;
        float distance = glm::length(toOther);

        if (distance > effectiveRange || distance < 0.1f) continue;

        float angle = atan2(toOther.z, toOther.x) - facing;
        angle = normalizeAngle(angle);

        if (!isInFieldOfView(angle, 0.0f, genome.visionFOV)) continue;

        // Get target's camouflage level from the creature's genome
        float targetCamo = other->getCamouflageLevel();

        float targetSpeed = glm::length(other->getVelocity());

        float probability = calculateDetectionProbability(
            distance, effectiveRange, targetCamo, targetSpeed, environment);

        // Motion detection bonus
        if (targetSpeed > 0.5f) {
            probability += genome.motionDetection * 0.3f * (targetSpeed / 10.0f);
            probability = std::min(1.0f, probability);
        }

        if (probability > 0.1f) {
            SensoryPercept percept;

            // Determine detection type based on creature type
            CreatureType otherType = other->getType();
            // This will need creature reference to determine relationship
            // For now, we'll set basic types
            percept.type = (otherType == CreatureType::CARNIVORE) ?
                           DetectionType::PREDATOR : DetectionType::PREY;

            percept.position = other->getPosition();
            percept.velocity = other->getVelocity();
            percept.distance = distance;
            percept.angle = angle;
            percept.confidence = probability;
            percept.strength = 1.0f - (distance / effectiveRange);
            percept.sensedBy = SensoryType::VISION;
            percept.sourceCreature = other;
            percept.timestamp = currentTime;
            currentPercepts.push_back(percept);
        }
    }
}

void SensorySystem::senseHearing(
    const glm::vec3& position,
    const std::vector<SoundEvent>& sounds,
    const std::vector<Creature*>& creatures,
    const EnvironmentConditions& environment,
    float currentTime
) {
    float effectiveRange = genome.hearingRange;
    if (environment.isUnderwater) {
        effectiveRange *= 4.0f;  // Sound travels better underwater
    }

    // Process explicit sound events
    for (const auto& sound : sounds) {
        glm::vec3 toSound = sound.position - position;
        float distance = glm::length(toSound);

        if (distance > effectiveRange || distance < 0.1f) continue;

        float attenuation = calculateSoundAttenuation(distance, sound.frequency, environment);
        float perceivedIntensity = sound.intensity * attenuation;

        if (perceivedIntensity < 0.05f) continue;

        SensoryPercept percept;

        // Determine type based on sound type
        switch (sound.type) {
            case SoundType::ALARM_CALL:
                percept.type = DetectionType::DANGER_ZONE;
                break;
            case SoundType::MATING_CALL:
                percept.type = DetectionType::MATE;
                break;
            case SoundType::MOVEMENT:
            case SoundType::FEEDING:
            default:
                percept.type = DetectionType::SOUND_SOURCE;
                break;
        }

        percept.position = sound.position;
        percept.velocity = glm::vec3(0.0f);
        percept.distance = distance;

        float angle = atan2(toSound.z, toSound.x);
        // Add directional uncertainty based on hearing quality
        float uncertainty = (1.0f - genome.hearingDirectionality) * 0.5f;
        percept.angle = angle;
        percept.confidence = genome.hearingDirectionality * perceivedIntensity;
        percept.strength = perceivedIntensity;
        percept.sensedBy = SensoryType::HEARING;
        percept.sourceCreature = sound.source;
        percept.timestamp = currentTime;

        currentPercepts.push_back(percept);
    }

    // Passive hearing of nearby creature movement (footsteps, etc.)
    for (Creature* other : creatures) {
        if (!other || !other->isAlive()) continue;

        float speed = glm::length(other->getVelocity());
        if (speed < 1.0f) continue;  // Stationary creatures are silent

        glm::vec3 toOther = other->getPosition() - position;
        float distance = glm::length(toOther);

        if (distance > effectiveRange * 0.5f || distance < 0.1f) continue;

        // Larger/faster creatures are louder
        float soundIntensity = (speed / 15.0f) * (other->getGenome().size);
        float attenuation = calculateSoundAttenuation(distance, 500.0f, environment);
        float perceivedIntensity = soundIntensity * attenuation;

        if (perceivedIntensity < 0.1f) continue;

        SensoryPercept percept;
        percept.type = DetectionType::MOVEMENT;
        percept.position = other->getPosition();
        percept.velocity = other->getVelocity();
        percept.distance = distance;
        percept.angle = atan2(toOther.z, toOther.x);
        percept.confidence = perceivedIntensity * genome.hearingDirectionality;
        percept.strength = perceivedIntensity;
        percept.sensedBy = SensoryType::HEARING;
        percept.sourceCreature = other;
        percept.timestamp = currentTime;

        currentPercepts.push_back(percept);
    }

    // Echolocation (if creature has the ability)
    if (genome.echolocationAbility > 0.3f) {
        float echoRange = genome.visionRange * genome.echolocationAbility;

        for (Creature* other : creatures) {
            if (!other || !other->isAlive()) continue;

            glm::vec3 toOther = other->getPosition() - position;
            float distance = glm::length(toOther);

            if (distance > echoRange || distance < 0.1f) continue;

            // Echolocation ignores visibility but uses energy
            float echoStrength = 1.0f - (distance / echoRange);
            echoStrength *= genome.echolocationAbility;

            if (echoStrength < 0.1f) continue;

            SensoryPercept percept;
            percept.type = (other->getType() == CreatureType::CARNIVORE) ?
                           DetectionType::PREDATOR : DetectionType::PREY;
            percept.position = other->getPosition();
            percept.velocity = other->getVelocity();
            percept.distance = distance;
            percept.angle = atan2(toOther.z, toOther.x);
            percept.confidence = echoStrength;
            percept.strength = echoStrength;
            percept.sensedBy = SensoryType::HEARING;  // Echolocation is hearing-based
            percept.sourceCreature = other;
            percept.timestamp = currentTime;

            currentPercepts.push_back(percept);
        }
    }
}

void SensorySystem::senseSmell(
    const glm::vec3& position,
    const std::vector<glm::vec3>& foodPositions,
    const std::vector<Creature*>& creatures,
    const EnvironmentConditions& environment,
    float currentTime
) {
    if (environment.isUnderwater) {
        // Smell works differently underwater - more like chemoreception
        // Reduced range but more accurate
    }

    float effectiveRange = genome.smellRange;

    // Detect food by smell
    for (const auto& foodPos : foodPositions) {
        glm::vec3 toFood = foodPos - position;
        float distance = glm::length(toFood);

        if (distance > effectiveRange || distance < 0.1f) continue;

        float scentStrength = calculateScentStrength(
            distance, environment.windDirection, environment.windSpeed, glm::normalize(toFood));

        scentStrength *= genome.smellSensitivity;

        if (scentStrength < 0.05f) continue;

        SensoryPercept percept;
        percept.type = DetectionType::FOOD;
        percept.position = foodPos;
        percept.velocity = glm::vec3(0.0f);
        percept.distance = distance;
        percept.angle = atan2(toFood.z, toFood.x);
        // Smell is less precise about direction than vision
        percept.confidence = scentStrength * 0.7f;
        percept.strength = scentStrength;
        percept.sensedBy = SensoryType::SMELL;
        percept.sourceCreature = nullptr;
        percept.timestamp = currentTime;

        currentPercepts.push_back(percept);
    }

    // Detect creatures by scent (predators, prey)
    for (Creature* other : creatures) {
        if (!other || !other->isAlive()) continue;

        glm::vec3 toOther = other->getPosition() - position;
        float distance = glm::length(toOther);

        if (distance > effectiveRange || distance < 0.1f) continue;

        float scentStrength = calculateScentStrength(
            distance, environment.windDirection, environment.windSpeed, glm::normalize(toOther));

        // Larger creatures have stronger scent
        scentStrength *= other->getGenome().size * genome.smellSensitivity;

        if (scentStrength < 0.05f) continue;

        SensoryPercept percept;
        percept.type = (other->getType() == CreatureType::CARNIVORE) ?
                       DetectionType::PREDATOR : DetectionType::CONSPECIFIC;
        percept.position = other->getPosition();
        percept.velocity = glm::vec3(0.0f);  // Can't smell velocity
        percept.distance = distance;
        percept.angle = atan2(toOther.z, toOther.x);
        percept.confidence = scentStrength * 0.5f;  // Less confident than vision
        percept.strength = scentStrength;
        percept.sensedBy = SensoryType::SMELL;
        percept.sourceCreature = other;
        percept.timestamp = currentTime;

        currentPercepts.push_back(percept);
    }
}

void SensorySystem::senseTouch(
    const glm::vec3& position,
    const std::vector<Creature*>& creatures,
    const EnvironmentConditions& environment,
    float currentTime
) {
    float effectiveRange = genome.touchRange;

    for (Creature* other : creatures) {
        if (!other || !other->isAlive()) continue;

        glm::vec3 toOther = other->getPosition() - position;
        float distance = glm::length(toOther);

        // Touch is very short range
        if (distance > effectiveRange || distance < 0.1f) continue;

        // Vibration detection extends range slightly
        float vibrationRange = effectiveRange * (1.0f + genome.vibrationSensitivity * 2.0f);

        float touchStrength = 1.0f - (distance / effectiveRange);

        // Vibration detection - movement creates vibrations
        float speed = glm::length(other->getVelocity());
        float vibrationStrength = 0.0f;
        if (distance < vibrationRange && speed > 0.5f) {
            vibrationStrength = (speed / 10.0f) * genome.vibrationSensitivity;
            vibrationStrength *= 1.0f - (distance / vibrationRange);
        }

        float totalStrength = std::max(touchStrength, vibrationStrength);

        if (totalStrength < 0.1f) continue;

        SensoryPercept percept;
        percept.type = DetectionType::MOVEMENT;
        percept.position = other->getPosition();
        percept.velocity = other->getVelocity();
        percept.distance = distance;
        percept.angle = atan2(toOther.z, toOther.x);
        percept.confidence = totalStrength;
        percept.strength = totalStrength;
        percept.sensedBy = SensoryType::TOUCH;
        percept.sourceCreature = other;
        percept.timestamp = currentTime;

        currentPercepts.push_back(percept);
    }
}

std::vector<SensoryPercept> SensorySystem::getPerceptsByType(DetectionType type) const {
    std::vector<SensoryPercept> result;
    for (const auto& percept : currentPercepts) {
        if (percept.type == type) {
            result.push_back(percept);
        }
    }
    return result;
}

std::vector<SensoryPercept> SensorySystem::getPerceptsBySense(SensoryType sense) const {
    std::vector<SensoryPercept> result;
    for (const auto& percept : currentPercepts) {
        if (percept.sensedBy == sense) {
            result.push_back(percept);
        }
    }
    return result;
}

bool SensorySystem::hasThreatNearby() const {
    for (const auto& percept : currentPercepts) {
        if (percept.type == DetectionType::PREDATOR ||
            percept.type == DetectionType::DANGER_ZONE) {
            return true;
        }
    }
    return false;
}

bool SensorySystem::hasFoodNearby() const {
    for (const auto& percept : currentPercepts) {
        if (percept.type == DetectionType::FOOD) {
            return true;
        }
    }
    return false;
}

SensoryPercept SensorySystem::getNearestThreat() const {
    SensoryPercept nearest;
    nearest.distance = std::numeric_limits<float>::max();

    for (const auto& percept : currentPercepts) {
        if ((percept.type == DetectionType::PREDATOR ||
             percept.type == DetectionType::DANGER_ZONE) &&
            percept.distance < nearest.distance) {
            nearest = percept;
        }
    }

    return nearest;
}

SensoryPercept SensorySystem::getNearestFood() const {
    SensoryPercept nearest;
    nearest.distance = std::numeric_limits<float>::max();

    for (const auto& percept : currentPercepts) {
        if (percept.type == DetectionType::FOOD && percept.distance < nearest.distance) {
            nearest = percept;
        }
    }

    return nearest;
}

SensoryPercept SensorySystem::getNearestMate() const {
    SensoryPercept nearest;
    nearest.distance = std::numeric_limits<float>::max();

    for (const auto& percept : currentPercepts) {
        if (percept.type == DetectionType::MATE && percept.distance < nearest.distance) {
            nearest = percept;
        }
    }

    return nearest;
}

std::vector<float> SensorySystem::generateNeuralInputs() const {
    std::vector<float> inputs;
    inputs.reserve(20);

    // Find nearest of each type for neural network input
    auto nearestFood = getNearestFood();
    auto nearestThreat = getNearestThreat();
    auto nearestMate = getNearestMate();

    // Vision inputs (6)
    inputs.push_back(nearestFood.distance < 1000.0f ?
                     1.0f - (nearestFood.distance / genome.visionRange) : 0.0f);
    inputs.push_back(nearestFood.distance < 1000.0f ?
                     nearestFood.angle / 3.14159f : 0.0f);
    inputs.push_back(nearestThreat.distance < 1000.0f ?
                     1.0f - (nearestThreat.distance / genome.visionRange) : 0.0f);
    inputs.push_back(nearestThreat.distance < 1000.0f ?
                     nearestThreat.angle / 3.14159f : 0.0f);
    inputs.push_back(nearestMate.distance < 1000.0f ?
                     1.0f - (nearestMate.distance / genome.visionRange) : 0.0f);
    inputs.push_back(nearestMate.distance < 1000.0f ?
                     nearestMate.angle / 3.14159f : 0.0f);

    // Hearing inputs (4)
    auto hearingPercepts = getPerceptsBySense(SensoryType::HEARING);
    float loudestSound = 0.0f;
    float loudestAngle = 0.0f;
    bool alarmDetected = false;
    for (const auto& p : hearingPercepts) {
        if (p.strength > loudestSound) {
            loudestSound = p.strength;
            loudestAngle = p.angle;
        }
        if (p.type == DetectionType::DANGER_ZONE) {
            alarmDetected = true;
        }
    }
    inputs.push_back(loudestSound);
    inputs.push_back(loudestAngle / 3.14159f);
    inputs.push_back(alarmDetected ? 1.0f : 0.0f);
    inputs.push_back(genome.echolocationAbility);  // Echo capability

    // Smell inputs (4)
    auto smellPercepts = getPerceptsBySense(SensoryType::SMELL);
    float strongestScent = 0.0f;
    float scentAngle = 0.0f;
    float predatorScent = 0.0f;
    for (const auto& p : smellPercepts) {
        if (p.type == DetectionType::FOOD && p.strength > strongestScent) {
            strongestScent = p.strength;
            scentAngle = p.angle;
        }
        if (p.type == DetectionType::PREDATOR) {
            predatorScent = std::max(predatorScent, p.strength);
        }
    }
    inputs.push_back(strongestScent);
    inputs.push_back(scentAngle / 3.14159f);
    inputs.push_back(predatorScent);
    inputs.push_back(0.0f);  // Pheromone trail direction (placeholder)

    // Touch inputs (2)
    auto touchPercepts = getPerceptsBySense(SensoryType::TOUCH);
    float nearbyMovement = 0.0f;
    bool contact = false;
    for (const auto& p : touchPercepts) {
        nearbyMovement = std::max(nearbyMovement, p.strength);
        if (p.distance < 1.0f) contact = true;
    }
    inputs.push_back(nearbyMovement);
    inputs.push_back(contact ? 1.0f : 0.0f);

    // Memory inputs (2)
    inputs.push_back(memory.hasMemoryOf(MemoryType::FOOD_LOCATION) ? 1.0f : 0.0f);
    inputs.push_back(memory.hasMemoryOf(MemoryType::DANGER_LOCATION) ? 1.0f : 0.0f);

    return inputs;
}

void SensorySystem::updateMemory(float deltaTime) {
    memory.update(deltaTime);

    // Remember important percepts
    for (const auto& percept : currentPercepts) {
        if (percept.type == DetectionType::FOOD && percept.confidence > 0.5f) {
            memory.remember(percept.position, MemoryType::FOOD_LOCATION, 0.6f);
        }
        if (percept.type == DetectionType::PREDATOR && percept.confidence > 0.5f) {
            memory.remember(percept.position, MemoryType::DANGER_LOCATION, 0.8f);
        }
        if (percept.type == DetectionType::DANGER_ZONE) {
            memory.remember(percept.position, MemoryType::DANGER_LOCATION, 0.9f);
        }
    }
}

void SensorySystem::emitAlarmCall(std::vector<SoundEvent>& soundBuffer, const glm::vec3& position, float timestamp) {
    if (genome.alarmCallVolume < 0.1f) return;

    SoundEvent alarm;
    alarm.position = position;
    alarm.type = SoundType::ALARM_CALL;
    alarm.intensity = genome.alarmCallVolume;
    alarm.frequency = 2000.0f;  // High frequency alarm
    alarm.timestamp = timestamp;
    alarm.source = nullptr;  // Will be set by caller

    soundBuffer.push_back(alarm);
}

void SensorySystem::emitMatingCall(std::vector<SoundEvent>& soundBuffer, const glm::vec3& position, float timestamp) {
    if (genome.displayIntensity < 0.1f) return;

    SoundEvent call;
    call.position = position;
    call.type = SoundType::MATING_CALL;
    call.intensity = genome.displayIntensity;
    call.frequency = 500.0f;  // Lower frequency mating call
    call.timestamp = timestamp;
    call.source = nullptr;

    soundBuffer.push_back(call);
}

// Helper functions

float SensorySystem::calculateDetectionProbability(
    float distance,
    float maxRange,
    float targetCamouflage,
    float targetSpeed,
    const EnvironmentConditions& environment
) const {
    if (distance >= maxRange) return 0.0f;

    // Base probability from distance (exponential falloff)
    float rangeRatio = distance / maxRange;
    float baseProb = std::exp(-rangeRatio * rangeRatio * 2.0f);

    // Environmental visibility modifier
    baseProb *= environment.visibility;

    // Camouflage reduces detection
    baseProb *= (1.0f - targetCamouflage * 0.8f);

    // Acuity affects detection at range
    baseProb *= (0.5f + genome.visionAcuity * 0.5f);

    return std::clamp(baseProb, 0.0f, 1.0f);
}

float SensorySystem::normalizeAngle(float angle) const {
    while (angle > 3.14159f) angle -= 2.0f * 3.14159f;
    while (angle < -3.14159f) angle += 2.0f * 3.14159f;
    return angle;
}

bool SensorySystem::isInFieldOfView(float angleToTarget, float facing, float fov) const {
    float relativeAngle = normalizeAngle(angleToTarget - facing);
    return std::abs(relativeAngle) <= (fov / 2.0f);
}

float SensorySystem::calculateSoundAttenuation(float distance, float frequency, const EnvironmentConditions& env) const {
    // Inverse square law with medium-dependent absorption
    float inverseSquare = 1.0f / (1.0f + distance * distance * 0.01f);

    // High frequencies attenuate faster
    float freqFactor = 1.0f - (frequency / 20000.0f) * 0.3f;

    // Sound travels better in water
    if (env.isUnderwater) {
        inverseSquare *= 2.0f;
    }

    return inverseSquare * freqFactor;
}

float SensorySystem::calculateScentStrength(float distance, const glm::vec3& windDir, float windSpeed,
                                             const glm::vec3& toTarget) const {
    // Base attenuation with distance
    float baseStrength = 1.0f / (1.0f + distance * 0.05f);

    // Wind affects scent propagation
    // Downwind from source = stronger scent
    // Upwind from source = weaker scent
    float windDot = glm::dot(glm::normalize(glm::vec3(windDir.x, 0.0f, windDir.z)),
                             glm::vec3(toTarget.x, 0.0f, toTarget.z));

    // If wind is blowing from target to us (windDot > 0), scent is stronger
    float windModifier = 1.0f + windDot * windSpeed * 0.2f;
    windModifier = std::clamp(windModifier, 0.2f, 2.0f);

    return baseStrength * windModifier;
}

// ============================================================================
// PheromoneGrid Implementation
// ============================================================================

PheromoneGrid::PheromoneGrid(float worldSize, float cellSize)
    : worldSize(worldSize), cellSize(cellSize),
      evaporationRate(0.05f), diffusionRate(0.02f) {
    gridSize = static_cast<int>(worldSize / cellSize);
    grid.resize(gridSize * gridSize);
}

void PheromoneGrid::deposit(const glm::vec3& position, PheromoneType type, float strength) {
    int idx = positionToIndex(position);
    if (idx >= 0 && idx < static_cast<int>(grid.size())) {
        grid[idx].concentrations[type] += strength;
        grid[idx].concentrations[type] = std::min(1.0f, grid[idx].concentrations[type]);
    }
}

float PheromoneGrid::sample(const glm::vec3& position, PheromoneType type) const {
    int idx = positionToIndex(position);
    if (idx >= 0 && idx < static_cast<int>(grid.size())) {
        auto it = grid[idx].concentrations.find(type);
        if (it != grid[idx].concentrations.end()) {
            return it->second;
        }
    }
    return 0.0f;
}

glm::vec3 PheromoneGrid::getGradient(const glm::vec3& position, PheromoneType type) const {
    float dx = sample(position + glm::vec3(cellSize, 0, 0), type) -
               sample(position - glm::vec3(cellSize, 0, 0), type);
    float dz = sample(position + glm::vec3(0, 0, cellSize), type) -
               sample(position - glm::vec3(0, 0, cellSize), type);

    glm::vec3 gradient(dx, 0.0f, dz);
    if (glm::length(gradient) > 0.001f) {
        gradient = glm::normalize(gradient);
    }
    return gradient;
}

void PheromoneGrid::update(float deltaTime) {
    // Simple evaporation
    for (auto& cell : grid) {
        for (auto& [type, concentration] : cell.concentrations) {
            concentration *= (1.0f - evaporationRate * deltaTime);
            if (concentration < 0.01f) {
                concentration = 0.0f;
            }
        }
    }
}

void PheromoneGrid::clear() {
    for (auto& cell : grid) {
        cell.concentrations.clear();
    }
}

int PheromoneGrid::positionToIndex(const glm::vec3& position) const {
    float halfWorld = worldSize / 2.0f;
    int x = static_cast<int>((position.x + halfWorld) / cellSize);
    int z = static_cast<int>((position.z + halfWorld) / cellSize);

    x = std::clamp(x, 0, gridSize - 1);
    z = std::clamp(z, 0, gridSize - 1);

    return z * gridSize + x;
}

glm::vec3 PheromoneGrid::indexToPosition(int index) const {
    int x = index % gridSize;
    int z = index / gridSize;
    float halfWorld = worldSize / 2.0f;

    return glm::vec3(
        x * cellSize - halfWorld + cellSize / 2.0f,
        0.0f,
        z * cellSize - halfWorld + cellSize / 2.0f
    );
}

// ============================================================================
// SoundManager Implementation
// ============================================================================

SoundManager::SoundManager(float maxRange)
    : maxSoundDuration(2.0f), maxRange(maxRange) {}

void SoundManager::addSound(const SoundEvent& sound) {
    activeSounds.push_back(sound);
}

void SoundManager::update(float deltaTime) {
    // Age sounds and remove old ones
    for (auto& sound : activeSounds) {
        sound.timestamp += deltaTime;
    }

    activeSounds.erase(
        std::remove_if(activeSounds.begin(), activeSounds.end(),
            [this](const SoundEvent& s) { return s.timestamp > maxSoundDuration; }),
        activeSounds.end()
    );
}

void SoundManager::clear() {
    activeSounds.clear();
}

std::vector<SoundEvent> SoundManager::getSoundsInRange(const glm::vec3& position, float range) const {
    std::vector<SoundEvent> result;
    for (const auto& sound : activeSounds) {
        if (glm::length(sound.position - position) < range) {
            result.push_back(sound);
        }
    }
    return result;
}
