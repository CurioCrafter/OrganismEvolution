#include "Volcano.h"
#include "../../core/CreatureManager.h"
#include "../VegetationManager.h"
#include "../Terrain.h"

#include <algorithm>
#include <cmath>

using namespace Forge;

namespace disasters {

VolcanoDisaster::VolcanoDisaster() {
    m_lavaParticles.reserve(MAX_LAVA_PARTICLES);
    m_lavaFlows.reserve(16);
    m_pyroclasticFlows.reserve(8);
}

void VolcanoDisaster::trigger(const glm::vec3& position, float radius, DisasterSeverity severity) {
    m_active = true;
    m_position = position;
    m_radius = radius;
    m_severity = severity;
    m_progress = 0.0f;

    // Initialize random generator
    std::random_device rd;
    m_rng.seed(rd());

    // Clear previous state
    m_lavaParticles.clear();
    m_lavaFlows.clear();
    m_pyroclasticFlows.clear();

    // Set parameters based on severity
    switch (severity) {
        case DisasterSeverity::MINOR:
            m_eruptionIntensity = 0.3f;
            m_baseHeatDamage = 3.0f;
            m_maxLavaFlows = 3;
            m_maxPyroclasticFlows = 1;
            break;
        case DisasterSeverity::MODERATE:
            m_eruptionIntensity = 0.6f;
            m_baseHeatDamage = 5.0f;
            m_maxLavaFlows = 5;
            m_maxPyroclasticFlows = 2;
            break;
        case DisasterSeverity::MAJOR:
            m_eruptionIntensity = 0.85f;
            m_baseHeatDamage = 8.0f;
            m_maxLavaFlows = 8;
            m_maxPyroclasticFlows = 3;
            break;
        case DisasterSeverity::CATASTROPHIC:
            m_eruptionIntensity = 1.0f;
            m_baseHeatDamage = 12.0f;
            m_maxLavaFlows = 12;
            m_maxPyroclasticFlows = 5;
            break;
    }

    // Initialize ash cloud
    m_ashCloud.position = position;
    m_ashCloud.position.y = position.y + 50.0f; // Above volcano
    m_ashCloud.radius = radius * 0.3f;
    m_ashCloud.density = 0.0f;
    m_ashCloud.altitude = 50.0f;
    m_ashCloud.spreadRate = ASH_SPREAD_RATE;

    // Initial burst of lava particles
    spawnLavaParticles(static_cast<int>(200 * m_eruptionIntensity));

    // Spawn initial lava flows
    int initialFlows = std::max(1, static_cast<int>(m_maxLavaFlows * 0.3f));
    for (int i = 0; i < initialFlows; ++i) {
        spawnLavaFlow();
    }
}

void VolcanoDisaster::update(float deltaTime, CreatureManager& creatures,
                             VegetationManager& vegetation, ActiveDisaster& disaster) {
    if (!m_active) return;

    m_progress = disaster.progress;

    // Update eruption intensity based on phase
    float targetIntensity;
    if (m_progress < 0.15f) {
        // Initial eruption - building
        targetIntensity = m_eruptionIntensity * (m_progress / 0.15f);
    } else if (m_progress < 0.60f) {
        // Active eruption - full intensity
        targetIntensity = m_eruptionIntensity;
    } else if (m_progress < 0.85f) {
        // Waning phase
        float waneProgress = (m_progress - 0.60f) / 0.25f;
        targetIntensity = m_eruptionIntensity * (1.0f - waneProgress * 0.6f);
    } else {
        // Cooling phase
        float coolProgress = (m_progress - 0.85f) / 0.15f;
        targetIntensity = m_eruptionIntensity * 0.4f * (1.0f - coolProgress);
    }

    // Smooth intensity changes
    m_eruptionIntensity = glm::mix(m_eruptionIntensity, targetIntensity, deltaTime * 2.0f);

    // Update all systems
    updateLavaFlows(deltaTime);
    updateLavaParticles(deltaTime);
    updatePyroclasticFlows(deltaTime);
    updateAshCloud(deltaTime);

    // Spawn new elements based on intensity
    m_particleSpawnAccumulator += deltaTime * m_eruptionIntensity;
    if (m_particleSpawnAccumulator > 0.05f) {
        spawnLavaParticles(static_cast<int>(20 * m_eruptionIntensity));
        m_particleSpawnAccumulator = 0.0f;
    }

    m_lavaSpawnTimer += deltaTime;
    if (m_lavaSpawnTimer > 3.0f / m_eruptionIntensity &&
        m_lavaFlows.size() < static_cast<size_t>(m_maxLavaFlows)) {
        spawnLavaFlow();
        m_lavaSpawnTimer = 0.0f;
    }

    m_pyroclasticSpawnTimer += deltaTime;
    if (m_pyroclasticSpawnTimer > 10.0f / m_eruptionIntensity &&
        m_pyroclasticFlows.size() < static_cast<size_t>(m_maxPyroclasticFlows) &&
        m_progress < 0.7f) { // Only in active phases
        spawnPyroclasticFlow();
        m_pyroclasticSpawnTimer = 0.0f;
    }

    // Apply effects
    applyCreatureDamage(creatures, deltaTime, disaster);
    destroyVegetation(vegetation, disaster);

    // Check if complete
    if (m_progress >= 1.0f) {
        m_active = false;
    }
}

void VolcanoDisaster::reset() {
    m_active = false;
    m_progress = 0.0f;
    m_eruptionIntensity = 0.0f;
    m_lavaParticles.clear();
    m_lavaFlows.clear();
    m_pyroclasticFlows.clear();
    m_ashCloud.density = 0.0f;
    m_ashCloud.radius = 0.0f;
}

float VolcanoDisaster::getEruptionIntensity() const {
    return m_eruptionIntensity;
}

float VolcanoDisaster::getTemperatureAt(const glm::vec3& position) const {
    if (!m_active) return 0.0f;

    float maxTemp = 0.0f;

    // Check distance from epicenter
    float distFromCenter = glm::length(glm::vec2(position.x - m_position.x,
                                                  position.z - m_position.z));
    if (distFromCenter < m_radius) {
        float normalizedDist = distFromCenter / m_radius;
        float centerTemp = m_lavaTemperature * m_eruptionIntensity * (1.0f - normalizedDist);
        maxTemp = std::max(maxTemp, centerTemp);
    }

    // Check lava flows
    for (const auto& flow : m_lavaFlows) {
        if (!flow.active) continue;

        float distFromFlow = glm::length(position - flow.currentPosition);
        if (distFromFlow < flow.width * 2.0f) {
            float temp = flow.temperature * (1.0f - distFromFlow / (flow.width * 2.0f));
            maxTemp = std::max(maxTemp, temp);
        }
    }

    // Check pyroclastic flows
    for (const auto& pyro : m_pyroclasticFlows) {
        if (!pyro.active) continue;

        float distFromPyro = glm::length(position - pyro.currentPosition);
        if (distFromPyro < pyro.radius) {
            float temp = pyro.temperature * (1.0f - distFromPyro / pyro.radius);
            maxTemp = std::max(maxTemp, temp);
        }
    }

    return maxTemp;
}

bool VolcanoDisaster::isInDangerZone(const glm::vec3& position) const {
    if (!m_active) return false;

    // Basic radius check
    float dist = glm::length(glm::vec2(position.x - m_position.x, position.z - m_position.z));
    if (dist < m_radius * 0.3f) return true; // Definite danger near center

    // Check lava flows
    for (const auto& flow : m_lavaFlows) {
        if (!flow.active) continue;
        float flowDist = glm::length(position - flow.currentPosition);
        if (flowDist < flow.width * 1.5f) return true;
    }

    // Check pyroclastic flows
    for (const auto& pyro : m_pyroclasticFlows) {
        if (!pyro.active) continue;
        float pyroDist = glm::length(position - pyro.currentPosition);
        if (pyroDist < pyro.radius) return true;
    }

    return false;
}

void VolcanoDisaster::updateLavaFlows(float deltaTime) {
    for (auto& flow : m_lavaFlows) {
        if (!flow.active) continue;

        // Move flow
        flow.currentPosition += flow.direction * flow.speed * deltaTime;
        flow.length += flow.speed * deltaTime;

        // Record path point
        if (flow.pathPoints.empty() ||
            glm::length(flow.currentPosition - flow.pathPoints.back()) > 2.0f) {
            flow.pathPoints.push_back(flow.currentPosition);
        }

        // Cool down over distance
        float distFromOrigin = glm::length(flow.currentPosition - flow.origin);
        flow.temperature = m_lavaTemperature * (1.0f - distFromOrigin / (m_radius * 2.0f));

        // Deactivate if cooled or too far
        if (flow.temperature < 400.0f || distFromOrigin > m_radius * 1.5f) {
            flow.active = false;
        }
    }

    // Clean up inactive flows
    m_lavaFlows.erase(
        std::remove_if(m_lavaFlows.begin(), m_lavaFlows.end(),
            [](const LavaFlow& f) { return !f.active && f.temperature < 100.0f; }),
        m_lavaFlows.end());
}

void VolcanoDisaster::updateLavaParticles(float deltaTime) {
    for (auto& particle : m_lavaParticles) {
        if (!particle.active) continue;

        // Physics update
        particle.velocity.y -= 9.8f * deltaTime; // Gravity
        particle.position += particle.velocity * deltaTime;
        particle.lifetime -= deltaTime;

        // Cool down
        particle.temperature -= deltaTime * 100.0f;

        // Update color based on temperature
        float tempNorm = (particle.temperature - 600.0f) / 600.0f;
        particle.color = glm::mix(
            glm::vec3(0.3f, 0.0f, 0.0f),  // Cooled (dark red)
            glm::vec3(1.0f, 0.8f, 0.2f),   // Hot (yellow-orange)
            std::clamp(tempNorm, 0.0f, 1.0f)
        );

        // Deactivate if expired or landed
        if (particle.lifetime <= 0.0f || particle.position.y < 0.0f) {
            particle.active = false;
        }
    }

    // Remove inactive particles periodically
    if (m_lavaParticles.size() > MAX_LAVA_PARTICLES * 0.9f) {
        m_lavaParticles.erase(
            std::remove_if(m_lavaParticles.begin(), m_lavaParticles.end(),
                [](const LavaParticle& p) { return !p.active; }),
            m_lavaParticles.end());
    }
}

void VolcanoDisaster::updatePyroclasticFlows(float deltaTime) {
    for (auto& pyro : m_pyroclasticFlows) {
        if (!pyro.active) continue;

        // Move rapidly outward
        pyro.currentPosition += pyro.direction * pyro.speed * deltaTime;
        pyro.radius += deltaTime * 2.0f; // Expand as it travels

        // Cool down
        pyro.temperature -= deltaTime * 50.0f;

        // Check if dissipated
        float dist = glm::length(pyro.currentPosition - pyro.origin);
        if (pyro.temperature < 200.0f || dist > m_radius * 2.0f) {
            pyro.active = false;
        }
    }

    // Remove inactive
    m_pyroclasticFlows.erase(
        std::remove_if(m_pyroclasticFlows.begin(), m_pyroclasticFlows.end(),
            [](const PyroclasticFlow& p) { return !p.active; }),
        m_pyroclasticFlows.end());
}

void VolcanoDisaster::updateAshCloud(float deltaTime) {
    if (m_progress < 0.15f) {
        // Build up
        m_ashCloud.density = std::min(1.0f, m_ashCloud.density + deltaTime * 0.5f);
        m_ashCloud.radius += m_ashCloud.spreadRate * deltaTime * 2.0f;
    } else if (m_progress > 0.85f) {
        // Settle
        m_ashCloud.density = std::max(0.0f, m_ashCloud.density - deltaTime * 0.2f);
        m_ashCloud.altitude = std::max(10.0f, m_ashCloud.altitude - deltaTime * 5.0f);
    } else {
        // Maintain and spread
        m_ashCloud.radius += m_ashCloud.spreadRate * deltaTime * m_eruptionIntensity;
        m_ashCloud.radius = std::min(m_ashCloud.radius, m_radius * 3.0f);
    }
}

void VolcanoDisaster::spawnLavaParticles(int count) {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> speedDist(5.0f, 25.0f);
    std::uniform_real_distribution<float> verticalDist(15.0f, 40.0f);
    std::uniform_real_distribution<float> sizeDist(0.3f, 1.5f);
    std::uniform_real_distribution<float> lifetimeDist(2.0f, 6.0f);

    for (int i = 0; i < count && m_lavaParticles.size() < MAX_LAVA_PARTICLES; ++i) {
        LavaParticle particle;

        float angle = angleDist(m_rng);
        float speed = speedDist(m_rng) * m_eruptionIntensity;
        float verticalSpeed = verticalDist(m_rng) * m_eruptionIntensity;

        particle.position = m_position;
        particle.position.y += 5.0f; // Start above ground

        particle.velocity.x = std::cos(angle) * speed;
        particle.velocity.y = verticalSpeed;
        particle.velocity.z = std::sin(angle) * speed;

        particle.temperature = 900.0f + (m_rng() % 300);
        particle.maxLifetime = lifetimeDist(m_rng);
        particle.lifetime = particle.maxLifetime;
        particle.size = sizeDist(m_rng);
        particle.color = glm::vec3(1.0f, 0.6f, 0.1f);
        particle.active = true;

        m_lavaParticles.push_back(particle);
    }
}

void VolcanoDisaster::spawnLavaFlow() {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> speedDist(0.5f, 1.5f);

    LavaFlow flow;
    float angle = angleDist(m_rng);

    flow.origin = m_position;
    flow.currentPosition = m_position;
    flow.direction = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
    flow.temperature = m_lavaTemperature;
    flow.width = 3.0f + (m_rng() % 5);
    flow.length = 0.0f;
    flow.speed = LAVA_FLOW_SPEED * speedDist(m_rng);
    flow.active = true;
    flow.pathPoints.push_back(flow.origin);

    m_lavaFlows.push_back(flow);
}

void VolcanoDisaster::spawnPyroclasticFlow() {
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);

    PyroclasticFlow pyro;
    float angle = angleDist(m_rng);

    pyro.origin = m_position;
    pyro.currentPosition = m_position;
    pyro.direction = glm::normalize(glm::vec3(std::cos(angle), 0.0f, std::sin(angle)));
    pyro.radius = 5.0f;
    pyro.speed = PYROCLASTIC_SPEED;
    pyro.temperature = 450.0f + (m_rng() % 250);
    pyro.active = true;

    m_pyroclasticFlows.push_back(pyro);
}

void VolcanoDisaster::applyCreatureDamage(CreatureManager& creatures, float deltaTime,
                                          ActiveDisaster& disaster) {
    // Query all creatures in the danger zone
    const auto& nearbyCreatures = creatures.queryNearby(m_position, m_radius * 2.0f);

    for (auto* creature : nearbyCreatures) {
        if (!creature || !creature->isAlive()) continue;

        float damage = calculateHeatDamage(creature->getPosition(), deltaTime);

        if (damage > 0.0f) {
            creature->takeDamage(damage);
            disaster.creaturesAffected++;

            if (!creature->isAlive()) {
                disaster.creaturesKilled++;
            }
        }
    }
}

float VolcanoDisaster::calculateHeatDamage(const glm::vec3& creaturePos, float deltaTime) const {
    float totalDamage = 0.0f;

    // Distance from epicenter
    float distFromCenter = glm::length(glm::vec2(creaturePos.x - m_position.x,
                                                  creaturePos.z - m_position.z));

    if (distFromCenter < m_radius * 0.5f) {
        // Close to crater - severe heat
        float normalizedDist = distFromCenter / (m_radius * 0.5f);
        totalDamage += m_baseHeatDamage * m_eruptionIntensity * (1.0f - normalizedDist);
    }

    // Check proximity to lava flows
    for (const auto& flow : m_lavaFlows) {
        if (!flow.active) continue;

        // Check distance to flow path
        for (const auto& pathPoint : flow.pathPoints) {
            float dist = glm::length(creaturePos - pathPoint);
            if (dist < flow.width * 2.0f) {
                float flowDamage = (flow.temperature / m_lavaTemperature) *
                                   m_baseHeatDamage * (1.0f - dist / (flow.width * 2.0f));
                totalDamage = std::max(totalDamage, flowDamage);
            }
        }
    }

    // Check pyroclastic flows - VERY DANGEROUS
    for (const auto& pyro : m_pyroclasticFlows) {
        if (!pyro.active) continue;

        float dist = glm::length(creaturePos - pyro.currentPosition);
        if (dist < pyro.radius) {
            // Pyroclastic flows are nearly always lethal
            float pyroDamage = m_baseHeatDamage * 3.0f * (1.0f - dist / pyro.radius);
            totalDamage = std::max(totalDamage, pyroDamage);
        }
    }

    return totalDamage * deltaTime;
}

glm::vec3 VolcanoDisaster::calculateFlowDirection(const glm::vec3& position) const {
    // In a real implementation, this would use terrain height to flow downhill
    // For now, just flow radially outward from volcano
    glm::vec3 dir = position - m_position;
    dir.y = 0.0f;
    if (glm::length(dir) > 0.001f) {
        return glm::normalize(dir);
    }
    return glm::vec3(1.0f, 0.0f, 0.0f);
}

void VolcanoDisaster::destroyVegetation(VegetationManager& vegetation, ActiveDisaster& disaster) {
    // Get tree instances
    auto& trees = const_cast<std::vector<TreeInstance>&>(vegetation.getTreeInstances());
    auto& bushes = const_cast<std::vector<BushInstance>&>(vegetation.getBushInstances());

    // Destroy trees in lava zone
    int treesDestroyed = 0;
    for (auto it = trees.begin(); it != trees.end(); ) {
        float dist = glm::length(it->position - m_position);

        // Check if in active lava area
        bool inLava = false;
        if (dist < m_radius * 0.3f * m_eruptionIntensity) {
            inLava = true;
        } else {
            // Check lava flows
            for (const auto& flow : m_lavaFlows) {
                if (!flow.active) continue;
                for (const auto& point : flow.pathPoints) {
                    if (glm::length(it->position - point) < flow.width) {
                        inLava = true;
                        break;
                    }
                }
                if (inLava) break;
            }
        }

        if (inLava) {
            it = trees.erase(it);
            treesDestroyed++;
        } else {
            ++it;
        }
    }

    // Destroy bushes similarly
    int bushesDestroyed = 0;
    for (auto it = bushes.begin(); it != bushes.end(); ) {
        float dist = glm::length(it->position - m_position);

        bool inLava = false;
        if (dist < m_radius * 0.4f * m_eruptionIntensity) {
            inLava = true;
        } else {
            for (const auto& flow : m_lavaFlows) {
                if (!flow.active) continue;
                for (const auto& point : flow.pathPoints) {
                    if (glm::length(it->position - point) < flow.width * 1.5f) {
                        inLava = true;
                        break;
                    }
                }
                if (inLava) break;
            }
        }

        if (inLava) {
            it = bushes.erase(it);
            bushesDestroyed++;
        } else {
            ++it;
        }
    }

    disaster.vegetationDestroyed += treesDestroyed + bushesDestroyed;
}

} // namespace disasters
