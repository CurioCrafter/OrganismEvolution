#include "GPUBehaviorCompute.h"
#include "../entities/Creature.h"
#include "../entities/CreatureType.h"
#include <chrono>
#include <algorithm>
#include <cmath>

#ifdef USE_FORGE_ENGINE
#include "DX12DeviceAdapter.h"
#else
#include "../graphics/DX12Device.h"
#endif

namespace Forge {

// ============================================================================
// Initialization
// ============================================================================

bool GPUBehaviorCompute::initialize(DX12Device* device) {
    if (m_initialized) return true;
    if (!device) return false;

    m_device = device;

    // For now, we use CPU-based behavior computation
    // GPU pipelines can be added when HLSL shaders are ready
    m_initialized = true;
    return true;
}

void GPUBehaviorCompute::shutdown() {
    m_device = nullptr;
    m_baseCompute = nullptr;
    m_initialized = false;
}

// ============================================================================
// Data Preparation
// ============================================================================

void GPUBehaviorCompute::prepareCreatureData(const std::vector<Creature*>& creatures) {
    auto startTime = std::chrono::high_resolution_clock::now();

    m_creatureData.clear();
    m_creatureData.reserve(creatures.size());
    m_outputCache.resize(creatures.size());

    for (size_t i = 0; i < creatures.size(); ++i) {
        if (creatures[i] && creatures[i]->isActive()) {
            m_creatureData.push_back(convertCreature(creatures[i], i));
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.dataUploadTimeMs = duration.count() / 1000.0f;
    m_stats.creaturesProcessed = static_cast<uint32_t>(m_creatureData.size());
}

BehaviorCreatureData GPUBehaviorCompute::convertCreature(const Creature* creature, size_t index) {
    BehaviorCreatureData data = {};

    // Position and velocity
    glm::vec3 pos = creature->getPosition();
    data.position = { pos.x, pos.y, pos.z };

    glm::vec3 vel = creature->getVelocity();
    data.velocity = { vel.x, vel.y, vel.z };

    // State
    data.energy = creature->getEnergy();
    data.fear = creature->getFear();
    data.isAlive = creature->isActive() ? 1 : 0;

    // Type info
    CreatureType type = creature->getType();
    data.type = static_cast<uint32_t>(type);
    data.isPredator = isPredator(type) ? 1 : 0;

    // Determine active behaviors based on creature state
    uint32_t behaviors = 0;
    if (isFlocking(type)) {
        behaviors |= static_cast<uint32_t>(BehaviorType::FLOCKING);
    }
    if (isPredator(type)) {
        behaviors |= static_cast<uint32_t>(BehaviorType::PREDATOR);
    }
    if (!isPredator(type)) {
        behaviors |= static_cast<uint32_t>(BehaviorType::PREY);
    }
    if (creature->getEnergy() < m_config.hungerThreshold) {
        behaviors |= static_cast<uint32_t>(BehaviorType::FORAGING);
    }
    data.behaviorFlags = behaviors;

    // Extended attributes from genome
    const Genome& genome = creature->getGenome();
    data.size = creature->getSize();
    data.speed = genome.speed;
    data.senseRadius = genome.senseRadius;
    data.age = creature->getAge();

    // Social attributes
    data.speciesID = static_cast<uint32_t>(type);
    data.packID = 0;  // Could be set from social system
    data.territoryRadius = 10.0f;  // Default
    data.socialWeight = 1.0f;

    // Migration (if applicable)
    data.migrationTarget = { 0.0f, 0.0f, 0.0f };
    data.migrationUrgency = 0.0f;

    return data;
}

// ============================================================================
// Data Updates
// ============================================================================

void GPUBehaviorCompute::updateFlockingData(const std::vector<glm::vec3>& neighborPositions,
                                             const std::vector<glm::vec3>& neighborVelocities) {
    // Store for CPU fallback or future GPU upload
    // This data is integrated into spatial queries
}

void GPUBehaviorCompute::updatePredatorData(const std::vector<glm::vec3>& predatorPositions) {
    // Update predator position cache
}

void GPUBehaviorCompute::updateFoodData(const std::vector<glm::vec3>& foodPositions,
                                         const std::vector<float>& foodAmounts) {
    // Update food position cache
}

void GPUBehaviorCompute::updateMigrationTargets(const std::vector<glm::vec3>& targets) {
    // Update migration target cache
}

// ============================================================================
// Compute Dispatch
// ============================================================================

void GPUBehaviorCompute::dispatchAll(ID3D12GraphicsCommandList* cmdList, float deltaTime) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Reset stats
    m_stats.flockingCount = 0;
    m_stats.predatorCount = 0;
    m_stats.preyCount = 0;
    m_stats.foragingCount = 0;
    m_stats.migratingCount = 0;

    // Use CPU fallback for now (GPU shaders can be added later)
    computeFlockingCPU(deltaTime);
    computePredatorPreyCPU(deltaTime);
    computeForagingCPU(deltaTime);

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.computeTimeMs = duration.count() / 1000.0f;
}

void GPUBehaviorCompute::dispatchBehavior(ID3D12GraphicsCommandList* cmdList,
                                           BehaviorType behavior, float deltaTime) {
    // Dispatch specific behavior type
    if ((behavior & BehaviorType::FLOCKING) != BehaviorType::NONE) {
        computeFlockingCPU(deltaTime);
    }
    if ((behavior & BehaviorType::PREDATOR) != BehaviorType::NONE ||
        (behavior & BehaviorType::PREY) != BehaviorType::NONE) {
        computePredatorPreyCPU(deltaTime);
    }
    if ((behavior & BehaviorType::FORAGING) != BehaviorType::NONE) {
        computeForagingCPU(deltaTime);
    }
}

// ============================================================================
// CPU Fallback Implementations
// ============================================================================

void GPUBehaviorCompute::computeFlockingCPU(float deltaTime) {
    // Simple O(n^2) flocking - in production, use spatial grid
    const float sepDist = m_config.separationDistance;
    const float alignDist = m_config.alignmentDistance;
    const float cohDist = m_config.cohesionDistance;

    for (size_t i = 0; i < m_creatureData.size(); ++i) {
        const auto& creature = m_creatureData[i];
        if (!(creature.behaviorFlags & static_cast<uint32_t>(BehaviorType::FLOCKING))) {
            continue;
        }

        m_stats.flockingCount++;

        glm::vec3 separation(0.0f);
        glm::vec3 alignment(0.0f);
        glm::vec3 cohesion(0.0f);
        int sepCount = 0, alignCount = 0, cohCount = 0;

        glm::vec3 myPos(creature.position.x, creature.position.y, creature.position.z);
        glm::vec3 myVel(creature.velocity.x, creature.velocity.y, creature.velocity.z);

        for (size_t j = 0; j < m_creatureData.size(); ++j) {
            if (i == j) continue;

            const auto& other = m_creatureData[j];
            if (other.speciesID != creature.speciesID) continue;  // Same species only

            glm::vec3 otherPos(other.position.x, other.position.y, other.position.z);
            glm::vec3 otherVel(other.velocity.x, other.velocity.y, other.velocity.z);

            float dist = glm::length(otherPos - myPos);

            // Separation
            if (dist > 0.001f && dist < sepDist) {
                glm::vec3 diff = myPos - otherPos;
                separation += glm::normalize(diff) / dist;
                sepCount++;
            }

            // Alignment
            if (dist < alignDist) {
                alignment += otherVel;
                alignCount++;
            }

            // Cohesion
            if (dist < cohDist) {
                cohesion += otherPos;
                cohCount++;
            }
        }

        // Calculate steering forces
        glm::vec3 steer(0.0f);

        if (sepCount > 0) {
            separation /= static_cast<float>(sepCount);
            if (glm::length(separation) > 0.001f) {
                separation = glm::normalize(separation) * m_config.maxSpeed - myVel;
                if (glm::length(separation) > m_config.maxForce) {
                    separation = glm::normalize(separation) * m_config.maxForce;
                }
            }
            steer += separation * m_config.separationWeight;
        }

        if (alignCount > 0) {
            alignment /= static_cast<float>(alignCount);
            if (glm::length(alignment) > 0.001f) {
                alignment = glm::normalize(alignment) * m_config.maxSpeed - myVel;
                if (glm::length(alignment) > m_config.maxForce) {
                    alignment = glm::normalize(alignment) * m_config.maxForce;
                }
            }
            steer += alignment * m_config.alignmentWeight;
        }

        if (cohCount > 0) {
            cohesion /= static_cast<float>(cohCount);
            glm::vec3 desired = cohesion - myPos;
            if (glm::length(desired) > 0.001f) {
                desired = glm::normalize(desired) * m_config.maxSpeed - myVel;
                if (glm::length(desired) > m_config.maxForce) {
                    desired = glm::normalize(desired) * m_config.maxForce;
                }
            }
            steer += desired * m_config.cohesionWeight;
        }

        // Store results
        m_outputCache[i].steeringForce = { steer.x, steer.y, steer.z };
        m_outputCache[i].separationForce = { separation.x, separation.y, separation.z };
        m_outputCache[i].alignmentForce = { alignment.x, alignment.y, alignment.z };
        m_outputCache[i].cohesionForce = { cohesion.x, cohesion.y, cohesion.z };
        m_outputCache[i].separationWeight = m_config.separationWeight;
        m_outputCache[i].alignmentWeight = m_config.alignmentWeight;
        m_outputCache[i].cohesionWeight = m_config.cohesionWeight;
        m_outputCache[i].behaviorState |= static_cast<uint32_t>(BehaviorType::FLOCKING);
    }
}

void GPUBehaviorCompute::computePredatorPreyCPU(float deltaTime) {
    // Predator-prey interactions
    for (size_t i = 0; i < m_creatureData.size(); ++i) {
        const auto& creature = m_creatureData[i];

        glm::vec3 myPos(creature.position.x, creature.position.y, creature.position.z);
        glm::vec3 myVel(creature.velocity.x, creature.velocity.y, creature.velocity.z);

        if (creature.isPredator) {
            // Predator: seek nearest prey
            m_stats.predatorCount++;

            float closestDist = m_config.preyDetectionRange;
            glm::vec3 closestPrey(0.0f);
            bool foundPrey = false;

            for (size_t j = 0; j < m_creatureData.size(); ++j) {
                if (i == j) continue;
                const auto& other = m_creatureData[j];
                if (other.isPredator) continue;  // Skip other predators

                glm::vec3 otherPos(other.position.x, other.position.y, other.position.z);
                float dist = glm::length(otherPos - myPos);

                if (dist < closestDist) {
                    closestDist = dist;
                    closestPrey = otherPos;
                    foundPrey = true;
                }
            }

            if (foundPrey) {
                // Pursuit behavior with prediction
                glm::vec3 desired = closestPrey - myPos;
                float distance = glm::length(desired);

                if (distance > 0.001f) {
                    float speed = std::min(m_config.maxSpeed * 1.2f,
                                           m_config.maxSpeed * (distance / m_config.preyDetectionRange));
                    desired = glm::normalize(desired) * speed;

                    glm::vec3 steer = desired - myVel;
                    if (glm::length(steer) > m_config.maxForce) {
                        steer = glm::normalize(steer) * m_config.maxForce;
                    }

                    // Add to existing steering
                    m_outputCache[i].steeringForce.x += steer.x;
                    m_outputCache[i].steeringForce.y += steer.y;
                    m_outputCache[i].steeringForce.z += steer.z;
                    m_outputCache[i].priority = std::max(m_outputCache[i].priority, 0.9f);
                    m_outputCache[i].targetPosition = { closestPrey.x, closestPrey.y, closestPrey.z };
                    m_outputCache[i].targetType = 2;  // prey
                }
            }
            m_outputCache[i].behaviorState |= static_cast<uint32_t>(BehaviorType::PREDATOR);
        } else {
            // Prey: flee from nearest predator
            m_stats.preyCount++;

            float closestDist = m_config.predatorDetectionRange;
            glm::vec3 closestPredator(0.0f);
            bool foundPredator = false;

            for (size_t j = 0; j < m_creatureData.size(); ++j) {
                if (i == j) continue;
                const auto& other = m_creatureData[j];
                if (!other.isPredator) continue;

                glm::vec3 otherPos(other.position.x, other.position.y, other.position.z);
                float dist = glm::length(otherPos - myPos);

                if (dist < closestDist) {
                    closestDist = dist;
                    closestPredator = otherPos;
                    foundPredator = true;
                }
            }

            if (foundPredator) {
                // Flee behavior
                glm::vec3 fleeDir = myPos - closestPredator;
                float dist = glm::length(fleeDir);

                if (dist > 0.001f) {
                    // Flee strength inversely proportional to distance
                    float fleeStrength = (m_config.fleeDistance - dist) / m_config.fleeDistance;
                    fleeStrength = std::max(0.0f, fleeStrength);

                    glm::vec3 desired = glm::normalize(fleeDir) * m_config.maxSpeed * (1.0f + fleeStrength);
                    glm::vec3 steer = desired - myVel;

                    if (glm::length(steer) > m_config.maxForce * 1.5f) {
                        steer = glm::normalize(steer) * m_config.maxForce * 1.5f;
                    }

                    // Fleeing has high priority
                    m_outputCache[i].steeringForce.x += steer.x;
                    m_outputCache[i].steeringForce.y += steer.y;
                    m_outputCache[i].steeringForce.z += steer.z;
                    m_outputCache[i].priority = std::max(m_outputCache[i].priority, 0.95f);
                    m_outputCache[i].targetType = 3;  // flee
                    m_outputCache[i].urgency = fleeStrength;
                }
            }
            m_outputCache[i].behaviorState |= static_cast<uint32_t>(BehaviorType::PREY);
        }
    }
}

void GPUBehaviorCompute::computeForagingCPU(float deltaTime) {
    // Foraging behavior is handled by existing food-seeking in creature update
    // This is a placeholder for more advanced foraging algorithms
    for (size_t i = 0; i < m_creatureData.size(); ++i) {
        const auto& creature = m_creatureData[i];
        if (!(creature.behaviorFlags & static_cast<uint32_t>(BehaviorType::FORAGING))) {
            continue;
        }

        m_stats.foragingCount++;
        m_outputCache[i].behaviorState |= static_cast<uint32_t>(BehaviorType::FORAGING);
    }
}

// ============================================================================
// Results Interface
// ============================================================================

void GPUBehaviorCompute::readbackResults(std::vector<BehaviorOutput>& results) {
    auto startTime = std::chrono::high_resolution_clock::now();

    results = m_outputCache;

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    m_stats.readbackTimeMs = duration.count() / 1000.0f;
}

} // namespace Forge
