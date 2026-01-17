#include "Flood.h"
#include "../../core/CreatureManager.h"
#include "../Terrain.h"

#include <algorithm>
#include <cmath>

using namespace Forge;

namespace disasters {

Flood::Flood() {
    m_floodedAreas.reserve(20);
}

void Flood::trigger(const glm::vec3& epicenter, DisasterSeverity severity) {
    m_active = true;
    m_epicenter = epicenter;
    m_severity = severity;
    m_currentPhase = FloodPhase::RISING;
    m_progress = 0.0f;
    m_phaseTimer = 0.0f;
    m_currentWaterRise = 0.0f;

    std::random_device rd;
    m_rng.seed(rd());

    // Configure based on severity
    switch (severity) {
        case DisasterSeverity::MINOR:
            m_targetWaterRise = 8.0f;
            m_floodRadius = 60.0f;
            m_riseRate = 1.0f;
            m_baseDrowningDamage = 3.0f;
            m_risingDuration = 20.0f;
            m_sustainedDuration = 30.0f;
            break;
        case DisasterSeverity::MODERATE:
            m_targetWaterRise = 15.0f;
            m_floodRadius = 100.0f;
            m_riseRate = 1.5f;
            m_baseDrowningDamage = 5.0f;
            m_risingDuration = 30.0f;
            m_sustainedDuration = 60.0f;
            break;
        case DisasterSeverity::MAJOR:
            m_targetWaterRise = 25.0f;
            m_floodRadius = 150.0f;
            m_riseRate = 2.0f;
            m_baseDrowningDamage = 8.0f;
            m_risingDuration = 40.0f;
            m_sustainedDuration = 90.0f;
            break;
        case DisasterSeverity::CATASTROPHIC:
            m_targetWaterRise = 40.0f;
            m_floodRadius = 250.0f;
            m_riseRate = 3.0f;
            m_baseDrowningDamage = 12.0f;
            m_risingDuration = 50.0f;
            m_sustainedDuration = 120.0f;
            break;
    }

    // Initialize flooded areas (multiple flood zones)
    m_floodedAreas.clear();
    std::uniform_real_distribution<float> offsetDist(-m_floodRadius * 0.5f, m_floodRadius * 0.5f);
    int numAreas = 3 + static_cast<int>(severity);

    for (int i = 0; i < numAreas; ++i) {
        FloodedArea area;
        area.center = epicenter + glm::vec3(offsetDist(m_rng), 0.0f, offsetDist(m_rng));
        area.currentLevel = 0.0f;
        area.maxLevel = m_targetWaterRise * (0.7f + (m_rng() % 60) / 100.0f);
        area.radius = m_floodRadius * (0.6f + (m_rng() % 80) / 100.0f);
        area.active = true;
        m_floodedAreas.push_back(area);
    }
}

void Flood::update(float deltaTime, CreatureManager& creatures, Terrain& terrain,
                   ActiveDisaster& disaster) {
    if (!m_active) return;

    m_phaseTimer += deltaTime;

    switch (m_currentPhase) {
        case FloodPhase::RISING:
            updateRisingPhase(deltaTime, disaster);
            break;
        case FloodPhase::PEAK:
            updatePeakPhase(deltaTime, disaster);
            break;
        case FloodPhase::SUSTAINED:
            updateSustainedPhase(deltaTime, disaster);
            break;
        case FloodPhase::RECEDING:
            updateRecedingPhase(deltaTime, disaster);
            break;
        case FloodPhase::AFTERMATH:
            updateAftermathPhase(deltaTime, disaster);
            break;
    }

    // Update individual flooded areas
    updateFloodedAreas(deltaTime);

    // Apply effects to creatures
    applyCreatureEffects(creatures, terrain, deltaTime, disaster);

    // Calculate overall progress
    float totalDuration = m_risingDuration + m_peakDuration + m_sustainedDuration +
                          m_recedingDuration + m_aftermathDuration;
    float elapsed = 0.0f;

    switch (m_currentPhase) {
        case FloodPhase::RISING:
            elapsed = m_phaseTimer;
            break;
        case FloodPhase::PEAK:
            elapsed = m_risingDuration + m_phaseTimer;
            break;
        case FloodPhase::SUSTAINED:
            elapsed = m_risingDuration + m_peakDuration + m_phaseTimer;
            break;
        case FloodPhase::RECEDING:
            elapsed = m_risingDuration + m_peakDuration + m_sustainedDuration + m_phaseTimer;
            break;
        case FloodPhase::AFTERMATH:
            elapsed = m_risingDuration + m_peakDuration + m_sustainedDuration +
                      m_recedingDuration + m_phaseTimer;
            break;
    }

    disaster.progress = std::clamp(elapsed / totalDuration, 0.0f, 1.0f);
    m_progress = disaster.progress;

    // Update description
    disaster.description = "Flood - Water rise: " +
                          std::to_string(static_cast<int>(m_currentWaterRise)) + "m";
}

void Flood::updateRisingPhase(float deltaTime, ActiveDisaster& disaster) {
    float riseProgress = m_phaseTimer / m_risingDuration;
    m_currentWaterRise = m_targetWaterRise * riseProgress;

    if (m_phaseTimer >= m_risingDuration) {
        advancePhase(disaster);
    }
}

void Flood::updatePeakPhase(float deltaTime, ActiveDisaster& disaster) {
    // Maintain peak water level with slight fluctuation
    std::uniform_real_distribution<float> fluctuation(-0.5f, 0.5f);
    m_currentWaterRise = m_targetWaterRise + fluctuation(m_rng);

    if (m_phaseTimer >= m_peakDuration) {
        advancePhase(disaster);
    }
}

void Flood::updateSustainedPhase(float deltaTime, ActiveDisaster& disaster) {
    // Slight gradual decrease
    float sustainProgress = m_phaseTimer / m_sustainedDuration;
    m_currentWaterRise = m_targetWaterRise * (1.0f - sustainProgress * 0.1f);

    if (m_phaseTimer >= m_sustainedDuration) {
        advancePhase(disaster);
    }
}

void Flood::updateRecedingPhase(float deltaTime, ActiveDisaster& disaster) {
    float recedeProgress = m_phaseTimer / m_recedingDuration;
    m_currentWaterRise = m_targetWaterRise * 0.9f * (1.0f - recedeProgress);

    if (m_phaseTimer >= m_recedingDuration) {
        advancePhase(disaster);
    }
}

void Flood::updateAftermathPhase(float deltaTime, ActiveDisaster& disaster) {
    float aftermathProgress = m_phaseTimer / m_aftermathDuration;
    m_currentWaterRise = m_targetWaterRise * 0.1f * (1.0f - aftermathProgress);

    if (m_phaseTimer >= m_aftermathDuration || m_currentWaterRise < 0.5f) {
        m_active = false;
        disaster.progress = 1.0f;
    }
}

void Flood::advancePhase(ActiveDisaster& disaster) {
    m_phaseTimer = 0.0f;

    switch (m_currentPhase) {
        case FloodPhase::RISING:
            m_currentPhase = FloodPhase::PEAK;
            disaster.description = "Flood at peak - maximum water level";
            break;
        case FloodPhase::PEAK:
            m_currentPhase = FloodPhase::SUSTAINED;
            disaster.description = "Sustained flooding";
            break;
        case FloodPhase::SUSTAINED:
            m_currentPhase = FloodPhase::RECEDING;
            disaster.description = "Flood receding";
            break;
        case FloodPhase::RECEDING:
            m_currentPhase = FloodPhase::AFTERMATH;
            disaster.description = "Flood aftermath";
            break;
        case FloodPhase::AFTERMATH:
            break;
    }
}

void Flood::updateFloodedAreas(float deltaTime) {
    for (auto& area : m_floodedAreas) {
        if (!area.active) continue;

        // Update area water level based on phase
        switch (m_currentPhase) {
            case FloodPhase::RISING:
                area.currentLevel = std::min(area.maxLevel,
                    area.currentLevel + m_riseRate * deltaTime);
                break;
            case FloodPhase::PEAK:
            case FloodPhase::SUSTAINED:
                // Maintain level
                break;
            case FloodPhase::RECEDING:
            case FloodPhase::AFTERMATH:
                area.currentLevel = std::max(0.0f,
                    area.currentLevel - m_riseRate * 0.5f * deltaTime);
                if (area.currentLevel < 0.5f) {
                    area.active = false;
                }
                break;
        }
    }
}

void Flood::applyCreatureEffects(CreatureManager& creatures, const Terrain& terrain,
                                  float deltaTime, ActiveDisaster& disaster) {
    creatures.forEach([&](Creature& creature, size_t) {
        if (!creature.isAlive()) return;

        glm::vec3 pos = creature.getPosition();

        // Check if creature is in flooded area
        float floodDepth = getFloodDepth(pos, terrain);

        if (floodDepth <= 0.0f) return; // Not flooded

        // Determine if creature can swim (based on type)
        // In a real implementation, would check creature's aquatic traits
        bool canSwim = false; // Default: land creatures can't swim

        // Aquatic or amphibious creatures actually benefit from floods
        CreatureType type = creature.getType();
        if (type == CreatureType::FISH || type == CreatureType::AMPHIBIAN) {
            canSwim = true;
            // Give them an energy boost
            if (floodDepth > 1.0f) {
                creature.addEnergy(deltaTime * 0.5f);
            }
            return;
        }

        // Flying creatures can escape
        if (type == CreatureType::BIRD) {
            // Birds try to fly to safety - minimal damage
            if (floodDepth < 5.0f) return;
        }

        // Apply drowning damage to non-swimmers
        float damage = calculateDrowningDamage(floodDepth, creature.getEnergy(), canSwim);

        if (damage > 0.0f) {
            creature.takeDamage(damage * deltaTime);
            disaster.creaturesAffected++;

            if (!creature.isAlive()) {
                disaster.creaturesKilled++;
            }
        }
    });
}

float Flood::calculateDrowningDamage(float floodDepth, float creatureEnergy, bool canSwim) const {
    if (canSwim) return 0.0f;
    if (floodDepth < 0.5f) return 0.0f; // Shallow - no danger

    // Base drowning damage
    float damage = m_baseDrowningDamage;

    // Deeper water = more danger
    float depthFactor = std::min(3.0f, floodDepth / 5.0f);
    damage *= depthFactor;

    // Tired creatures drown faster
    if (creatureEnergy < 30.0f) {
        damage *= 1.5f;
    } else if (creatureEnergy > 70.0f) {
        damage *= 0.7f; // Can tread water longer
    }

    return damage;
}

float Flood::getWaterLevelAt(const glm::vec3& position) const {
    if (!m_active) return 0.0f;

    float maxLevel = 0.0f;

    for (const auto& area : m_floodedAreas) {
        if (!area.active) continue;

        float dist = glm::length(glm::vec2(position.x - area.center.x,
                                            position.z - area.center.z));

        if (dist < area.radius) {
            // Water level decreases toward edge
            float edgeFactor = 1.0f - (dist / area.radius) * 0.3f;
            float levelHere = area.currentLevel * edgeFactor;
            maxLevel = std::max(maxLevel, levelHere);
        }
    }

    return maxLevel;
}

bool Flood::isFlooded(const glm::vec3& position, const Terrain& terrain) const {
    float terrainHeight = terrain.getHeight(position.x, position.z);
    float waterLevel = getWaterLevelAt(position);

    return waterLevel > terrainHeight - position.y;
}

float Flood::getFloodDepth(const glm::vec3& position, const Terrain& terrain) const {
    float terrainHeight = terrain.getHeight(position.x, position.z);
    float waterLevel = getWaterLevelAt(position);

    // Original water level from terrain
    float baseWaterLevel = terrain.getWaterLevel();

    // Flood raises the water level
    float effectiveWaterLevel = baseWaterLevel + waterLevel;

    // Flood depth is how much water is above creature
    float depth = effectiveWaterLevel - terrainHeight;

    return std::max(0.0f, depth);
}

void Flood::reset() {
    m_active = false;
    m_currentPhase = FloodPhase::RISING;
    m_progress = 0.0f;
    m_phaseTimer = 0.0f;
    m_currentWaterRise = 0.0f;
    m_floodedAreas.clear();
}

} // namespace disasters
