#include "MeteorImpact.h"
#include "../../core/CreatureManager.h"
#include "../Terrain.h"
#include "../ClimateSystem.h"

#include <algorithm>
#include <cmath>

using namespace Forge;

namespace disasters {

MeteorImpact::MeteorImpact() {
    m_debris.reserve(MAX_DEBRIS);
}

void MeteorImpact::trigger(const glm::vec3& position, float radius, DisasterSeverity severity) {
    m_active = true;
    m_impactPosition = position;
    m_severity = severity;
    m_currentPhase = MeteorPhase::INCOMING;
    m_phaseTimer = 0.0f;

    std::random_device rd;
    m_rng.seed(rd());

    // Clear previous state
    m_debris.clear();

    // Configure based on severity
    switch (severity) {
        case DisasterSeverity::MINOR:
            m_meteorSize = 5.0f;
            m_baseDamage = 10.0f;
            m_maxShockwaveRadius = 40.0f;
            m_maxCooling = -5.0f;
            m_dustDuration = 60.0f;
            break;
        case DisasterSeverity::MODERATE:
            m_meteorSize = 15.0f;
            m_baseDamage = 20.0f;
            m_maxShockwaveRadius = 80.0f;
            m_maxCooling = -12.0f;
            m_dustDuration = 120.0f;
            break;
        case DisasterSeverity::MAJOR:
            m_meteorSize = 30.0f;
            m_baseDamage = 35.0f;
            m_maxShockwaveRadius = 150.0f;
            m_maxCooling = -20.0f;
            m_dustDuration = 200.0f;
            break;
        case DisasterSeverity::CATASTROPHIC:
            m_meteorSize = 50.0f;
            m_baseDamage = 50.0f;
            m_maxShockwaveRadius = 250.0f;
            m_maxCooling = -30.0f;
            m_dustDuration = 300.0f;
            break;
    }

    // Set up crater
    m_crater.center = position;
    m_crater.radius = radius;
    m_crater.depth = radius * 0.3f;
    m_crater.rimHeight = radius * 0.1f;
    m_crater.formed = false;

    // Set up meteor starting position (high in sky, approaching)
    m_meteorPosition = position + glm::vec3(100.0f, METEOR_START_ALTITUDE, 100.0f);
    m_meteorVelocity = glm::normalize(position - m_meteorPosition) * METEOR_APPROACH_SPEED;

    // Initialize shockwave (will activate on impact)
    m_shockwave.origin = position;
    m_shockwave.currentRadius = 0.0f;
    m_shockwave.maxRadius = m_maxShockwaveRadius;
    m_shockwave.speed = m_shockwaveSpeed;
    m_shockwave.intensity = 1.0f;
    m_shockwave.active = false;

    // Reset environmental effects
    m_dustCloudDensity = 0.0f;
    m_dustCloudRadius = 0.0f;
    m_sunlightReduction = 0.0f;
    m_temperatureOffset = 0.0f;
}

void MeteorImpact::update(float deltaTime, CreatureManager& creatures, Terrain& terrain,
                          ClimateSystem& climate, ActiveDisaster& disaster) {
    if (!m_active) return;

    m_phaseTimer += deltaTime;

    switch (m_currentPhase) {
        case MeteorPhase::INCOMING:
            updateIncomingPhase(deltaTime, disaster);
            break;
        case MeteorPhase::IMPACT:
            updateImpactPhase(deltaTime, creatures, disaster);
            break;
        case MeteorPhase::SHOCKWAVE:
            updateShockwavePhase(deltaTime, creatures, disaster);
            break;
        case MeteorPhase::DEBRIS:
            updateDebrisPhase(deltaTime, creatures, disaster);
            break;
        case MeteorPhase::DUST_CLOUD:
            updateDustCloudPhase(deltaTime, climate, disaster);
            break;
        case MeteorPhase::RECOVERY:
            updateRecoveryPhase(deltaTime, climate, disaster);
            break;
    }

    // Update debris particles
    for (auto& d : m_debris) {
        if (!d.active) continue;

        d.velocity.y -= 9.8f * deltaTime;
        d.position += d.velocity * deltaTime;
        d.lifetime -= deltaTime;

        if (d.lifetime <= 0.0f || d.position.y < terrain.getHeight(d.position.x, d.position.z)) {
            d.active = false;
        }
    }

    // Check completion
    if (disaster.progress >= 1.0f) {
        m_active = false;
    }
}

void MeteorImpact::reset() {
    m_active = false;
    m_currentPhase = MeteorPhase::INCOMING;
    m_phaseTimer = 0.0f;
    m_debris.clear();
    m_shockwave.active = false;
    m_dustCloudDensity = 0.0f;
    m_sunlightReduction = 0.0f;
    m_temperatureOffset = 0.0f;
}

void MeteorImpact::updateIncomingPhase(float deltaTime, ActiveDisaster& disaster) {
    // Move meteor toward impact point
    m_meteorPosition += m_meteorVelocity * deltaTime;

    // Check if meteor has reached impact
    float distToImpact = glm::length(m_meteorPosition - m_impactPosition);
    if (distToImpact < m_meteorSize) {
        advancePhase(disaster);
    }
}

void MeteorImpact::updateImpactPhase(float deltaTime, CreatureManager& creatures,
                                      ActiveDisaster& disaster) {
    // Form crater
    if (!m_crater.formed) {
        m_crater.formed = true;

        // Activate shockwave
        m_shockwave.active = true;
        m_shockwave.currentRadius = m_meteorSize;

        // Spawn initial debris burst
        spawnDebris(static_cast<int>(100 * (static_cast<int>(m_severity) + 1)));

        // Instant damage at ground zero
        const auto& nearbyCreatures = creatures.queryNearby(m_impactPosition, m_crater.radius * 0.5f);
        for (auto* creature : nearbyCreatures) {
            if (creature && creature->isAlive()) {
                // High damage but still survivable for creatures at edge
                float dist = glm::length(creature->getPosition() - m_impactPosition);
                float normalizedDist = dist / (m_crater.radius * 0.5f);
                float damage = m_baseDamage * 5.0f * (1.0f - normalizedDist * 0.5f);
                creature->takeDamage(damage);
                disaster.creaturesAffected++;
                if (!creature->isAlive()) {
                    disaster.creaturesKilled++;
                }
            }
        }
    }

    // Brief impact phase
    if (m_phaseTimer > 0.5f) {
        advancePhase(disaster);
    }
}

void MeteorImpact::updateShockwavePhase(float deltaTime, CreatureManager& creatures,
                                         ActiveDisaster& disaster) {
    if (!m_shockwave.active) {
        advancePhase(disaster);
        return;
    }

    // Expand shockwave
    m_shockwave.currentRadius += m_shockwave.speed * deltaTime;
    m_shockwave.intensity = 1.0f - (m_shockwave.currentRadius / m_shockwave.maxRadius);

    // Apply damage
    applyShockwaveDamage(creatures, deltaTime, disaster);

    // Continue spawning debris as shockwave expands
    if (m_rng() % 10 == 0) {
        spawnDebris(5);
    }

    // Check if shockwave exhausted
    if (m_shockwave.currentRadius >= m_shockwave.maxRadius) {
        m_shockwave.active = false;
        advancePhase(disaster);
    }
}

void MeteorImpact::updateDebrisPhase(float deltaTime, CreatureManager& creatures,
                                      ActiveDisaster& disaster) {
    // Continue debris rain
    if (m_phaseTimer < 10.0f) {
        spawnDebris(2);
    }

    // Apply debris damage
    applyDebrisDamage(creatures, deltaTime, disaster);

    // Build up dust cloud
    float targetDensity = 0.8f * (static_cast<int>(m_severity) + 1) / 4.0f;
    m_dustCloudDensity = std::min(targetDensity, m_dustCloudDensity + deltaTime * 0.1f);
    m_dustCloudRadius += deltaTime * 20.0f;

    // Advance when debris subsides
    if (m_phaseTimer > 15.0f) {
        advancePhase(disaster);
    }
}

void MeteorImpact::updateDustCloudPhase(float deltaTime, ClimateSystem& climate,
                                         ActiveDisaster& disaster) {
    // Continue dust cloud spreading
    m_dustCloudRadius = std::min(m_dustCloudRadius + deltaTime * 5.0f, 500.0f);

    // Calculate sunlight reduction and temperature
    float targetSunlightReduction = m_dustCloudDensity * 0.8f;
    m_sunlightReduction = glm::mix(m_sunlightReduction, targetSunlightReduction, deltaTime);

    float targetTempOffset = m_maxCooling * m_dustCloudDensity;
    m_temperatureOffset = glm::mix(m_temperatureOffset, targetTempOffset, deltaTime * 0.5f);

    // Apply to climate system
    // Note: Climate system integration - would use climate.setTemperatureModifier() if available

    // Gradual dust settling
    float phaseDuration = m_dustDuration;
    float phaseProgress = m_phaseTimer / phaseDuration;

    if (phaseProgress > 0.7f) {
        // Start settling
        float settleProgress = (phaseProgress - 0.7f) / 0.3f;
        m_dustCloudDensity *= (1.0f - settleProgress * 0.5f * deltaTime);
    }

    if (m_phaseTimer >= phaseDuration) {
        advancePhase(disaster);
    }
}

void MeteorImpact::updateRecoveryPhase(float deltaTime, ClimateSystem& climate,
                                        ActiveDisaster& disaster) {
    // Dust continues settling
    m_dustCloudDensity = std::max(0.0f, m_dustCloudDensity - deltaTime * 0.05f);
    m_sunlightReduction = std::max(0.0f, m_sunlightReduction - deltaTime * 0.02f);
    m_temperatureOffset = glm::mix(m_temperatureOffset, 0.0f, deltaTime * 0.1f);

    // Complete when effects normalized
    if (m_dustCloudDensity < 0.05f) {
        m_active = false;
    }
}

void MeteorImpact::advancePhase(ActiveDisaster& disaster) {
    m_phaseTimer = 0.0f;

    switch (m_currentPhase) {
        case MeteorPhase::INCOMING:
            m_currentPhase = MeteorPhase::IMPACT;
            disaster.description = "IMPACT!";
            break;
        case MeteorPhase::IMPACT:
            m_currentPhase = MeteorPhase::SHOCKWAVE;
            disaster.description = "Shockwave expanding";
            break;
        case MeteorPhase::SHOCKWAVE:
            m_currentPhase = MeteorPhase::DEBRIS;
            disaster.description = "Debris rain";
            break;
        case MeteorPhase::DEBRIS:
            m_currentPhase = MeteorPhase::DUST_CLOUD;
            disaster.description = "Nuclear winter";
            break;
        case MeteorPhase::DUST_CLOUD:
            m_currentPhase = MeteorPhase::RECOVERY;
            disaster.description = "Climate recovering";
            break;
        case MeteorPhase::RECOVERY:
            // Final phase - handled in update
            break;
    }
}

void MeteorImpact::spawnDebris(int count) {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> speedDist(10.0f, 50.0f);
    std::uniform_real_distribution<float> sizeDist(0.5f, 3.0f);
    std::uniform_real_distribution<float> lifetimeDist(3.0f, 8.0f);

    for (int i = 0; i < count && m_debris.size() < MAX_DEBRIS; ++i) {
        ImpactDebris debris;

        float angle = angleDist(m_rng);
        float elevation = angleDist(m_rng) * 0.25f + 0.25f; // 15-45 degrees up
        float speed = speedDist(m_rng);

        debris.position = m_impactPosition;
        debris.position.y += 10.0f;

        debris.velocity.x = std::cos(angle) * std::cos(elevation) * speed;
        debris.velocity.y = std::sin(elevation) * speed;
        debris.velocity.z = std::sin(angle) * std::cos(elevation) * speed;

        debris.size = sizeDist(m_rng);
        debris.lifetime = lifetimeDist(m_rng);
        debris.onFire = (m_rng() % 3 == 0);
        debris.active = true;

        m_debris.push_back(debris);
    }
}

void MeteorImpact::applyShockwaveDamage(CreatureManager& creatures, float deltaTime,
                                         ActiveDisaster& disaster) {
    if (!m_shockwave.active) return;

    // Get creatures in shockwave ring (current radius +/- width)
    float shockwaveWidth = 10.0f;
    float innerRadius = std::max(0.0f, m_shockwave.currentRadius - shockwaveWidth);
    float outerRadius = m_shockwave.currentRadius + shockwaveWidth;

    const auto& nearbyCreatures = creatures.queryNearby(m_shockwave.origin, outerRadius);

    for (auto* creature : nearbyCreatures) {
        if (!creature || !creature->isAlive()) continue;

        float dist = glm::length(creature->getPosition() - m_shockwave.origin);

        // Only affect creatures in the shockwave ring
        if (dist >= innerRadius && dist <= outerRadius) {
            float damage = calculateShockwaveDamage(dist) * deltaTime;
            creature->takeDamage(damage);
            disaster.creaturesAffected++;

            if (!creature->isAlive()) {
                disaster.creaturesKilled++;
            }
        }
    }
}

void MeteorImpact::applyDebrisDamage(CreatureManager& creatures, float deltaTime,
                                      ActiveDisaster& disaster) {
    // Check each active debris for creature collisions
    for (auto& debris : m_debris) {
        if (!debris.active) continue;

        const auto& nearbyCreatures = creatures.queryNearby(debris.position, debris.size * 2.0f);

        for (auto* creature : nearbyCreatures) {
            if (!creature || !creature->isAlive()) continue;

            // Debris hit - damage based on size
            float damage = debris.size * 5.0f;
            if (debris.onFire) damage *= 1.5f;

            creature->takeDamage(damage);
            disaster.creaturesAffected++;

            if (!creature->isAlive()) {
                disaster.creaturesKilled++;
            }

            // Debris is consumed on hit
            debris.active = false;
            break;
        }
    }
}

float MeteorImpact::calculateShockwaveDamage(float distance) const {
    // Damage decreases with distance and shockwave intensity
    float normalizedDist = distance / m_shockwave.maxRadius;
    float baseDamage = m_baseDamage * m_shockwave.intensity;

    // Shockwave damage falls off with inverse square
    return baseDamage / (1.0f + normalizedDist * normalizedDist);
}

} // namespace disasters
