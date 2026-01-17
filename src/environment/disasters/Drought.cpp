#include "Drought.h"
#include "../VegetationManager.h"
#include "../../core/CreatureManager.h"
#include "../Terrain.h"

#include <algorithm>
#include <cmath>

using namespace Forge;

namespace disasters {

Drought::Drought() {
    m_waterSources.reserve(100);
}

void Drought::trigger(DisasterSeverity severity) {
    m_active = true;
    m_severity = severity;
    m_currentPhase = DroughtPhase::DEVELOPING;
    m_progress = 0.0f;
    m_phaseTimer = 0.0f;
    m_currentSeverity = 0.0f;
    m_vegetationHealth = 1.0f;

    std::random_device rd;
    m_rng.seed(rd());

    // Configure based on severity
    switch (severity) {
        case DisasterSeverity::MINOR:
            m_targetSeverity = 0.4f;
            m_baseDehydrationDamage = 1.0f;
            m_developDuration = 30.0f;
            m_severeDuration = 60.0f;
            m_extremeDuration = 30.0f;
            break;
        case DisasterSeverity::MODERATE:
            m_targetSeverity = 0.6f;
            m_baseDehydrationDamage = 2.0f;
            m_developDuration = 45.0f;
            m_severeDuration = 90.0f;
            m_extremeDuration = 60.0f;
            break;
        case DisasterSeverity::MAJOR:
            m_targetSeverity = 0.8f;
            m_baseDehydrationDamage = 3.5f;
            m_developDuration = 60.0f;
            m_severeDuration = 120.0f;
            m_extremeDuration = 90.0f;
            break;
        case DisasterSeverity::CATASTROPHIC:
            m_targetSeverity = 0.95f;
            m_baseDehydrationDamage = 5.0f;
            m_developDuration = 90.0f;
            m_severeDuration = 180.0f;
            m_extremeDuration = 120.0f;
            break;
    }

    // Initialize some water sources that will shrink
    m_waterSources.clear();
    std::uniform_real_distribution<float> posDist(-150.0f, 150.0f);
    int numSources = 20;

    for (int i = 0; i < numSources; ++i) {
        glm::vec3 pos(posDist(m_rng), 0.0f, posDist(m_rng));
        pos.y = 5.0f; // Low-lying water sources
        m_waterSources.push_back(pos);
    }
}

void Drought::update(float deltaTime, VegetationManager& vegetation,
                     CreatureManager& creatures, Terrain& terrain, ActiveDisaster& disaster) {
    if (!m_active) return;

    m_phaseTimer += deltaTime;

    switch (m_currentPhase) {
        case DroughtPhase::DEVELOPING:
            updateDevelopingPhase(deltaTime, disaster);
            break;
        case DroughtPhase::SEVERE:
            updateSeverePhase(deltaTime, disaster);
            break;
        case DroughtPhase::EXTREME:
            updateExtremePhase(deltaTime, disaster);
            break;
        case DroughtPhase::BREAKING:
            updateBreakingPhase(deltaTime, disaster);
            break;
        case DroughtPhase::RECOVERY:
            updateRecoveryPhase(deltaTime, disaster);
            break;
    }

    // Apply effects
    applyVegetationEffects(vegetation, deltaTime, disaster);
    applyCreatureEffects(creatures, deltaTime, disaster);

    // Remove water sources as drought intensifies
    if (m_currentSeverity > 0.5f) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float removeChance = m_currentSeverity * deltaTime * 0.02f;

        m_waterSources.erase(
            std::remove_if(m_waterSources.begin(), m_waterSources.end(),
                [&](const glm::vec3&) { return dist(m_rng) < removeChance && m_waterSources.size() > 3; }),
            m_waterSources.end());
    }

    // Calculate overall progress
    float totalDuration = m_developDuration + m_severeDuration + m_extremeDuration +
                          m_breakingDuration + m_recoveryDuration;
    float elapsed = 0.0f;

    switch (m_currentPhase) {
        case DroughtPhase::DEVELOPING:
            elapsed = m_phaseTimer;
            break;
        case DroughtPhase::SEVERE:
            elapsed = m_developDuration + m_phaseTimer;
            break;
        case DroughtPhase::EXTREME:
            elapsed = m_developDuration + m_severeDuration + m_phaseTimer;
            break;
        case DroughtPhase::BREAKING:
            elapsed = m_developDuration + m_severeDuration + m_extremeDuration + m_phaseTimer;
            break;
        case DroughtPhase::RECOVERY:
            elapsed = m_developDuration + m_severeDuration + m_extremeDuration +
                      m_breakingDuration + m_phaseTimer;
            break;
    }

    disaster.progress = std::clamp(elapsed / totalDuration, 0.0f, 1.0f);
    m_progress = disaster.progress;

    // Update description
    disaster.description = "Drought - Severity: " +
                          std::to_string(static_cast<int>(m_currentSeverity * 100)) + "%, " +
                          "Water sources: " + std::to_string(m_waterSources.size());
}

void Drought::updateDevelopingPhase(float deltaTime, ActiveDisaster& disaster) {
    float developProgress = m_phaseTimer / m_developDuration;
    m_currentSeverity = m_targetSeverity * 0.3f * developProgress;
    m_vegetationHealth = 1.0f - developProgress * 0.2f;

    if (m_phaseTimer >= m_developDuration) {
        advancePhase(disaster);
    }
}

void Drought::updateSeverePhase(float deltaTime, ActiveDisaster& disaster) {
    float severeProgress = m_phaseTimer / m_severeDuration;
    m_currentSeverity = m_targetSeverity * (0.3f + 0.4f * severeProgress);
    m_vegetationHealth = 0.8f - severeProgress * 0.3f;

    if (m_phaseTimer >= m_severeDuration) {
        advancePhase(disaster);
    }
}

void Drought::updateExtremePhase(float deltaTime, ActiveDisaster& disaster) {
    float extremeProgress = m_phaseTimer / m_extremeDuration;
    m_currentSeverity = m_targetSeverity * (0.7f + 0.3f * extremeProgress);
    m_vegetationHealth = std::max(0.2f, 0.5f - extremeProgress * 0.3f);

    if (m_phaseTimer >= m_extremeDuration) {
        advancePhase(disaster);
    }
}

void Drought::updateBreakingPhase(float deltaTime, ActiveDisaster& disaster) {
    float breakProgress = m_phaseTimer / m_breakingDuration;
    m_currentSeverity = m_targetSeverity * (1.0f - breakProgress * 0.5f);
    m_vegetationHealth = std::min(0.5f, m_vegetationHealth + deltaTime * 0.01f);

    // Restore some water sources
    if (m_waterSources.size() < 10) {
        std::uniform_real_distribution<float> posDist(-150.0f, 150.0f);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(m_rng) < deltaTime * 0.1f) {
            glm::vec3 pos(posDist(m_rng), 5.0f, posDist(m_rng));
            m_waterSources.push_back(pos);
        }
    }

    if (m_phaseTimer >= m_breakingDuration) {
        advancePhase(disaster);
    }
}

void Drought::updateRecoveryPhase(float deltaTime, ActiveDisaster& disaster) {
    float recoveryProgress = m_phaseTimer / m_recoveryDuration;
    m_currentSeverity = m_targetSeverity * 0.5f * (1.0f - recoveryProgress);
    m_vegetationHealth = std::min(1.0f, m_vegetationHealth + deltaTime * 0.02f);

    if (m_phaseTimer >= m_recoveryDuration || m_currentSeverity < 0.05f) {
        m_active = false;
        disaster.progress = 1.0f;
    }
}

void Drought::advancePhase(ActiveDisaster& disaster) {
    m_phaseTimer = 0.0f;

    switch (m_currentPhase) {
        case DroughtPhase::DEVELOPING:
            m_currentPhase = DroughtPhase::SEVERE;
            disaster.description = "Drought intensifying - water scarce";
            break;
        case DroughtPhase::SEVERE:
            m_currentPhase = DroughtPhase::EXTREME;
            disaster.description = "Extreme drought - critical water shortage";
            break;
        case DroughtPhase::EXTREME:
            m_currentPhase = DroughtPhase::BREAKING;
            disaster.description = "Drought breaking - relief begins";
            break;
        case DroughtPhase::BREAKING:
            m_currentPhase = DroughtPhase::RECOVERY;
            disaster.description = "Drought recovery - conditions improving";
            break;
        case DroughtPhase::RECOVERY:
            break;
    }
}

void Drought::applyVegetationEffects(VegetationManager& vegetation, float deltaTime,
                                      ActiveDisaster& disaster) {
    if (m_currentSeverity < 0.2f) return;

    auto& trees = const_cast<std::vector<TreeInstance>&>(vegetation.getTreeInstances());
    auto& bushes = const_cast<std::vector<BushInstance>&>(vegetation.getBushInstances());

    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float dieOffChance = m_currentSeverity * deltaTime * 0.005f;

    int destroyed = 0;

    // Trees need water - die off based on distance from water
    for (auto it = trees.begin(); it != trees.end(); ) {
        float nearestWater = 1000.0f;
        for (const auto& water : m_waterSources) {
            float d = glm::length(it->position - water);
            nearestWater = std::min(nearestWater, d);
        }

        // Further from water = more likely to die
        float distanceFactor = std::min(1.0f, nearestWater / 100.0f);
        float chance = dieOffChance * (1.0f + distanceFactor);

        if (dist(m_rng) < chance) {
            it = trees.erase(it);
            destroyed++;
        } else {
            ++it;
        }
    }

    // Bushes are more vulnerable
    for (auto it = bushes.begin(); it != bushes.end(); ) {
        float nearestWater = 1000.0f;
        for (const auto& water : m_waterSources) {
            float d = glm::length(it->position - water);
            nearestWater = std::min(nearestWater, d);
        }

        float distanceFactor = std::min(1.0f, nearestWater / 80.0f);
        float chance = dieOffChance * 1.5f * (1.0f + distanceFactor);

        if (dist(m_rng) < chance) {
            it = bushes.erase(it);
            destroyed++;
        } else {
            ++it;
        }
    }

    disaster.vegetationDestroyed += destroyed;
}

void Drought::applyCreatureEffects(CreatureManager& creatures, float deltaTime,
                                    ActiveDisaster& disaster) {
    if (m_currentSeverity < 0.1f) return;

    creatures.forEach([&](Creature& creature, size_t) {
        if (!creature.isAlive()) return;

        glm::vec3 pos = creature.getPosition();
        float energy = creature.getEnergy();

        // Find nearest water
        float nearestWater = 1000.0f;
        for (const auto& water : m_waterSources) {
            float d = glm::length(pos - water);
            nearestWater = std::min(nearestWater, d);
        }

        float damage = calculateDehydrationDamage(energy, nearestWater);

        if (damage > 0.0f) {
            creature.takeDamage(damage * deltaTime);
            disaster.creaturesAffected++;

            if (!creature.isAlive()) {
                disaster.creaturesKilled++;
            }
        }
    });
}

float Drought::calculateDehydrationDamage(float energy, float distanceToWater) const {
    // No damage if near water
    if (distanceToWater < 20.0f) return 0.0f;

    // Base dehydration damage
    float damage = m_baseDehydrationDamage * m_currentSeverity;

    // Increases with distance from water
    float distanceFactor = std::min(2.0f, distanceToWater / 100.0f);
    damage *= (1.0f + distanceFactor);

    // Low energy creatures suffer more
    if (energy < 30.0f) {
        damage *= 1.5f;
    } else if (energy > 70.0f) {
        damage *= 0.7f; // Well-fed can endure longer
    }

    return damage;
}

bool Drought::hasWaterNearby(const glm::vec3& position, float searchRadius) const {
    for (const auto& water : m_waterSources) {
        if (glm::length(position - water) < searchRadius) {
            return true;
        }
    }
    return false;
}

float Drought::getDehydrationRate(const glm::vec3& position) const {
    if (!m_active) return 0.0f;

    float nearestWater = 1000.0f;
    for (const auto& water : m_waterSources) {
        float d = glm::length(position - water);
        nearestWater = std::min(nearestWater, d);
    }

    if (nearestWater < 20.0f) return 0.0f;

    return m_currentSeverity * (1.0f + nearestWater / 100.0f);
}

void Drought::reset() {
    m_active = false;
    m_currentPhase = DroughtPhase::DEVELOPING;
    m_progress = 0.0f;
    m_phaseTimer = 0.0f;
    m_currentSeverity = 0.0f;
    m_vegetationHealth = 1.0f;
    m_waterSources.clear();
}

} // namespace disasters
