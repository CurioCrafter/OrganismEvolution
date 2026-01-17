#include "IceAge.h"
#include "../ClimateSystem.h"
#include "../VegetationManager.h"
#include "../../core/CreatureManager.h"

#include <algorithm>
#include <cmath>

using namespace Forge;

namespace disasters {

IceAge::IceAge() {
    m_glaciers.reserve(50);
}

void IceAge::trigger(DisasterSeverity severity) {
    m_active = true;
    m_severity = severity;
    m_currentPhase = IceAgePhase::ONSET;
    m_progress = 0.0f;
    m_phaseTimer = 0.0f;

    std::random_device rd;
    m_rng.seed(rd());

    // Configure based on severity
    switch (severity) {
        case DisasterSeverity::MINOR:
            m_peakCooling = -10.0f;
            m_onsetDuration = 30.0f;
            m_plateauDuration = 120.0f;
            m_glacierLine = 70.0f;
            break;
        case DisasterSeverity::MODERATE:
            m_peakCooling = -20.0f;
            m_onsetDuration = 60.0f;
            m_plateauDuration = 240.0f;
            m_glacierLine = 50.0f;
            break;
        case DisasterSeverity::MAJOR:
            m_peakCooling = -30.0f;
            m_onsetDuration = 90.0f;
            m_plateauDuration = 400.0f;
            m_glacierLine = 30.0f;
            break;
        case DisasterSeverity::CATASTROPHIC:
            m_peakCooling = -45.0f;
            m_onsetDuration = 120.0f;
            m_plateauDuration = 600.0f;
            m_glacierLine = 10.0f;
            break;
    }

    m_targetTempModifier = m_peakCooling;
    m_currentTempModifier = 0.0f;
    m_iceCoverage = 0.0f;
    m_vegetationReduction = 0.0f;

    // Initialize glaciers at high points
    m_glaciers.clear();
    std::uniform_real_distribution<float> posDist(-200.0f, 200.0f);
    int numGlaciers = 10 + static_cast<int>(severity) * 10;

    for (int i = 0; i < numGlaciers; ++i) {
        Glacier glacier;
        glacier.position = glm::vec3(posDist(m_rng), m_glacierLine + 20.0f, posDist(m_rng));
        glacier.size = 5.0f;
        glacier.growthRate = 0.5f + (m_rng() % 100) / 100.0f;
        glacier.active = true;
        m_glaciers.push_back(glacier);
    }
}

void IceAge::update(float deltaTime, ClimateSystem& climate, VegetationManager& vegetation,
                    CreatureManager& creatures, ActiveDisaster& disaster) {
    if (!m_active) return;

    m_phaseTimer += deltaTime;

    switch (m_currentPhase) {
        case IceAgePhase::ONSET:
            updateOnsetPhase(deltaTime, climate, disaster);
            break;
        case IceAgePhase::PEAK:
            updatePeakPhase(deltaTime, climate, disaster);
            break;
        case IceAgePhase::PLATEAU:
            updatePlateauPhase(deltaTime, climate, disaster);
            break;
        case IceAgePhase::THAW:
            updateThawPhase(deltaTime, climate, disaster);
            break;
        case IceAgePhase::RECOVERY:
            updateRecoveryPhase(deltaTime, climate, disaster);
            break;
    }

    // Update glaciers
    updateGlaciers(deltaTime);

    // Apply effects to vegetation and creatures
    applyVegetationEffects(vegetation, deltaTime, disaster);
    applyCreatureEffects(creatures, deltaTime, disaster);

    // Calculate overall progress
    float totalDuration = m_onsetDuration + m_peakDuration + m_plateauDuration +
                          m_thawDuration + m_recoveryDuration;
    float elapsed = 0.0f;

    switch (m_currentPhase) {
        case IceAgePhase::ONSET:
            elapsed = m_phaseTimer;
            break;
        case IceAgePhase::PEAK:
            elapsed = m_onsetDuration + m_phaseTimer;
            break;
        case IceAgePhase::PLATEAU:
            elapsed = m_onsetDuration + m_peakDuration + m_phaseTimer;
            break;
        case IceAgePhase::THAW:
            elapsed = m_onsetDuration + m_peakDuration + m_plateauDuration + m_phaseTimer;
            break;
        case IceAgePhase::RECOVERY:
            elapsed = m_onsetDuration + m_peakDuration + m_plateauDuration +
                      m_thawDuration + m_phaseTimer;
            break;
    }

    disaster.progress = std::clamp(elapsed / totalDuration, 0.0f, 1.0f);
    m_progress = disaster.progress;

    // Update disaster description
    disaster.description = "Ice Age - Temp: " +
                          std::to_string(static_cast<int>(m_currentTempModifier)) + "°C, " +
                          "Ice: " + std::to_string(static_cast<int>(m_iceCoverage * 100)) + "%";
}

void IceAge::updateOnsetPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster) {
    // Gradual cooling
    float coolingProgress = m_phaseTimer / m_onsetDuration;
    m_currentTempModifier = m_peakCooling * coolingProgress * 0.7f;

    // Ice coverage increases
    m_iceCoverage = coolingProgress * 0.3f;

    // Vegetation starts dying
    m_vegetationReduction = coolingProgress * 0.2f;

    if (m_phaseTimer >= m_onsetDuration) {
        advancePhase(disaster);
    }
}

void IceAge::updatePeakPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster) {
    // Reach maximum cold
    float peakProgress = m_phaseTimer / m_peakDuration;
    m_currentTempModifier = m_peakCooling * (0.7f + peakProgress * 0.3f);

    // Ice coverage at peak
    m_iceCoverage = 0.3f + peakProgress * 0.4f;

    // Vegetation heavily impacted
    m_vegetationReduction = 0.2f + peakProgress * 0.4f;

    if (m_phaseTimer >= m_peakDuration) {
        advancePhase(disaster);
    }
}

void IceAge::updatePlateauPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster) {
    // Sustained cold with minor fluctuations
    std::uniform_real_distribution<float> fluctuation(-2.0f, 2.0f);
    m_currentTempModifier = m_peakCooling + fluctuation(m_rng);

    // Ice coverage at maximum
    m_iceCoverage = std::min(0.8f, m_iceCoverage + deltaTime * 0.01f);

    // Vegetation reduction sustained
    m_vegetationReduction = std::min(0.7f, m_vegetationReduction);

    if (m_phaseTimer >= m_plateauDuration) {
        advancePhase(disaster);
    }
}

void IceAge::updateThawPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster) {
    // Warming begins
    float thawProgress = m_phaseTimer / m_thawDuration;
    m_currentTempModifier = m_peakCooling * (1.0f - thawProgress * 0.6f);

    // Ice starts melting
    m_iceCoverage = std::max(0.2f, m_iceCoverage - deltaTime * 0.02f);

    // Vegetation begins recovery
    m_vegetationReduction = std::max(0.3f, m_vegetationReduction - deltaTime * 0.01f);

    if (m_phaseTimer >= m_thawDuration) {
        advancePhase(disaster);
    }
}

void IceAge::updateRecoveryPhase(float deltaTime, ClimateSystem& climate, ActiveDisaster& disaster) {
    // Return to normal
    float recoveryProgress = m_phaseTimer / m_recoveryDuration;
    m_currentTempModifier = m_peakCooling * 0.4f * (1.0f - recoveryProgress);

    // Ice melts
    m_iceCoverage = std::max(0.0f, m_iceCoverage - deltaTime * 0.03f);

    // Vegetation recovers
    m_vegetationReduction = std::max(0.0f, m_vegetationReduction - deltaTime * 0.02f);

    if (m_phaseTimer >= m_recoveryDuration || m_currentTempModifier > -1.0f) {
        m_active = false;
        disaster.progress = 1.0f;
    }
}

void IceAge::advancePhase(ActiveDisaster& disaster) {
    m_phaseTimer = 0.0f;

    switch (m_currentPhase) {
        case IceAgePhase::ONSET:
            m_currentPhase = IceAgePhase::PEAK;
            disaster.description = "Ice Age reaching peak cold";
            break;
        case IceAgePhase::PEAK:
            m_currentPhase = IceAgePhase::PLATEAU;
            disaster.description = "Ice Age plateau - sustained cold";
            break;
        case IceAgePhase::PLATEAU:
            m_currentPhase = IceAgePhase::THAW;
            disaster.description = "Ice Age thawing begins";
            break;
        case IceAgePhase::THAW:
            m_currentPhase = IceAgePhase::RECOVERY;
            disaster.description = "Climate recovering";
            break;
        case IceAgePhase::RECOVERY:
            // Final phase handled in update
            break;
    }
}

void IceAge::updateGlaciers(float deltaTime) {
    for (auto& glacier : m_glaciers) {
        if (!glacier.active) continue;

        if (m_currentPhase == IceAgePhase::ONSET || m_currentPhase == IceAgePhase::PEAK ||
            m_currentPhase == IceAgePhase::PLATEAU) {
            // Glaciers grow
            glacier.size += glacier.growthRate * deltaTime * (std::abs(m_currentTempModifier) / 30.0f);
            glacier.position.y = std::max(m_glacierLine - glacier.size * 0.5f, glacier.position.y - deltaTime * 0.1f);
        } else {
            // Glaciers shrink
            glacier.size -= glacier.growthRate * 0.5f * deltaTime;
            if (glacier.size < 1.0f) {
                glacier.active = false;
            }
        }
    }
}

void IceAge::applyVegetationEffects(VegetationManager& vegetation, float deltaTime,
                                    ActiveDisaster& disaster) {
    // Vegetation dies off in cold areas
    if (m_vegetationReduction < 0.1f) return;

    auto& trees = const_cast<std::vector<TreeInstance>&>(vegetation.getTreeInstances());
    auto& bushes = const_cast<std::vector<BushInstance>&>(vegetation.getBushInstances());

    // Probabilistic die-off based on cold intensity
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float dieOffChance = m_vegetationReduction * deltaTime * 0.01f;

    int destroyed = 0;

    // Trees more resistant than bushes
    for (auto it = trees.begin(); it != trees.end(); ) {
        // Higher altitude = more likely to die
        float altitude = it->position.y;
        float altitudeFactor = std::max(0.0f, (altitude - 20.0f) / 50.0f);
        float chance = dieOffChance * (0.5f + altitudeFactor);

        if (dist(m_rng) < chance) {
            it = trees.erase(it);
            destroyed++;
        } else {
            ++it;
        }
    }

    // Bushes more vulnerable
    for (auto it = bushes.begin(); it != bushes.end(); ) {
        float altitude = it->position.y;
        float altitudeFactor = std::max(0.0f, (altitude - 15.0f) / 40.0f);
        float chance = dieOffChance * (1.0f + altitudeFactor);

        if (dist(m_rng) < chance) {
            it = bushes.erase(it);
            destroyed++;
        } else {
            ++it;
        }
    }

    disaster.vegetationDestroyed += destroyed;
}

void IceAge::applyCreatureEffects(CreatureManager& creatures, float deltaTime,
                                   ActiveDisaster& disaster) {
    // Cold damage to creatures based on temperature
    float baseDamage = std::abs(m_currentTempModifier) * 0.02f; // Gradual, not instant

    creatures.forEach([&](Creature& creature, size_t) {
        if (!creature.isAlive()) return;

        float altitude = creature.getPosition().y;
        float energy = creature.getEnergy();

        // Calculate cold damage
        float damage = calculateColdDamage(m_currentTempModifier, energy);

        // Altitude makes it worse
        if (altitude > m_glacierLine) {
            damage *= 1.5f;
        }

        // Check if in glaciated area
        if (isGlaciated(creature.getPosition())) {
            damage *= 2.0f;
        }

        if (damage > 0.0f) {
            creature.takeDamage(damage * deltaTime);
            disaster.creaturesAffected++;

            if (!creature.isAlive()) {
                disaster.creaturesKilled++;
            }
        }
    });
}

float IceAge::calculateColdDamage(float temperature, float creatureEnergy) const {
    // Below freezing causes damage
    if (temperature >= 0.0f) return 0.0f;

    float coldIntensity = std::abs(temperature) / 30.0f; // Normalize to 0-1 range

    // Base damage scales with cold
    float damage = coldIntensity * 2.0f;

    // Low energy creatures suffer more
    if (creatureEnergy < 30.0f) {
        damage *= 1.5f;
    } else if (creatureEnergy > 70.0f) {
        damage *= 0.7f; // Well-fed creatures resist cold better
    }

    return damage;
}

bool IceAge::isGlaciated(const glm::vec3& position) const {
    if (!m_active) return false;

    // Check if above glacier line
    if (position.y > m_glacierLine) {
        return m_iceCoverage > 0.3f;
    }

    // Check proximity to glaciers
    for (const auto& glacier : m_glaciers) {
        if (!glacier.active) continue;

        float dist = glm::length(position - glacier.position);
        if (dist < glacier.size * 2.0f) {
            return true;
        }
    }

    return false;
}

void IceAge::reset() {
    m_active = false;
    m_currentPhase = IceAgePhase::ONSET;
    m_progress = 0.0f;
    m_phaseTimer = 0.0f;
    m_currentTempModifier = 0.0f;
    m_iceCoverage = 0.0f;
    m_vegetationReduction = 0.0f;
    m_glaciers.clear();
}

} // namespace disasters
