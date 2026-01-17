#include "TerritorialBehavior.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include "../../utils/SpatialGrid.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

void TerritorialBehavior::update(float deltaTime, CreatureManager& creatures, const SpatialGrid& grid) {
    m_currentTime += deltaTime;

    // First pass: remove territories of dead creatures
    removeDeadOwnerTerritories(creatures);

    // Second pass: update all active territories
    for (auto& [ownerID, territory] : m_territories) {
        Creature* owner = creatures.getCreatureByID(ownerID);
        if (!owner || !owner->isAlive()) {
            // Shouldn't happen after removeDeadOwnerTerritories, but be safe
            continue;
        }

        // Update territory center to drift toward owner position
        updateTerritoryCenter(territory, owner->getPosition(), deltaTime);

        // Increase territory strength over time (establishment)
        if (glm::distance(owner->getPosition(), territory.center) < territory.radius * 0.5f) {
            // Owner is within core territory - strengthen it
            territory.strength = glm::min(1.0f, territory.strength + m_config.strengthGainRate * deltaTime);
        } else if (glm::distance(owner->getPosition(), territory.center) > territory.radius) {
            // Owner is outside territory - weaken it
            territory.strength = glm::max(0.0f, territory.strength - m_config.strengthDecayRate * deltaTime);

            // If too far for too long, mark for removal
            if (glm::distance(owner->getPosition(), territory.center) > territory.radius * m_config.abandonDistance) {
                if (territory.strength < 0.1f) {
                    m_territoriesToRemove.push_back(ownerID);
                }
            }
        }

        // Process intrusions
        processIntrusions(territory, owner, grid, m_currentTime);

        // Decay intrusion count over time
        if (m_currentTime - territory.lastDefenseTime > 10.0f) {
            territory.intrusionCount = glm::max(0, territory.intrusionCount - 1);
            territory.lastDefenseTime = m_currentTime;
        }

        // Weaken old territories
        float age = m_currentTime - territory.establishedTime;
        if (age > m_config.maxTerritoryAge) {
            float ageFactor = (age - m_config.maxTerritoryAge) / m_config.maxTerritoryAge;
            territory.strength *= (1.0f - ageFactor * 0.1f * deltaTime);
        }
    }

    // Deferred removal of abandoned territories
    for (uint32_t id : m_territoriesToRemove) {
        m_territories.erase(id);
    }
    m_territoriesToRemove.clear();
}

glm::vec3 TerritorialBehavior::calculateForce(Creature* creature, const SpatialGrid& grid) {
    if (!creature || !creature->isAlive()) {
        return glm::vec3(0.0f);
    }

    uint32_t creatureID = creature->getID();

    // Check if this creature owns a territory
    auto it = m_territories.find(creatureID);
    if (it != m_territories.end()) {
        // Owner behavior: defend territory from intruders
        return calculateOwnerForce(creature, it->second, grid);
    } else {
        // Non-owner behavior: avoid other creatures' territories
        return calculateIntruderForce(creature);
    }
}

bool TerritorialBehavior::tryEstablishTerritory(Creature* creature, float resourceQuality) {
    if (!creature || !creature->isAlive()) {
        return false;
    }

    uint32_t creatureID = creature->getID();

    // Already has a territory
    if (m_territories.count(creatureID) > 0) {
        return false;
    }

    // Check energy requirement
    if (creature->getEnergy() < m_config.minEnergyToEstablish) {
        return false;
    }

    // Check if position overlaps with existing strong territories
    glm::vec3 pos = creature->getPosition();
    for (const auto& [otherID, territory] : m_territories) {
        if (territory.strength > 0.5f) {
            float dist = glm::distance(pos, territory.center);
            if (dist < territory.radius * 0.8f) {
                // Too close to established territory
                return false;
            }
        }
    }

    // Establish new territory
    Territory newTerritory;
    newTerritory.ownerID = creatureID;
    newTerritory.center = pos;
    newTerritory.radius = m_config.baseRadius + m_config.radiusPerSize * creature->getGenome().size;
    newTerritory.strength = 0.1f;  // Start weak
    newTerritory.establishedTime = m_currentTime;
    newTerritory.resourceQuality = resourceQuality;
    newTerritory.intrusionCount = 0;
    newTerritory.lastDefenseTime = m_currentTime;

    m_territories[creatureID] = newTerritory;
    return true;
}

void TerritorialBehavior::abandonTerritory(uint32_t creatureID) {
    m_territories.erase(creatureID);
}

bool TerritorialBehavior::hasTerritory(uint32_t creatureID) const {
    return m_territories.count(creatureID) > 0;
}

const TerritorialBehavior::Territory* TerritorialBehavior::getTerritory(uint32_t creatureID) const {
    auto it = m_territories.find(creatureID);
    if (it != m_territories.end()) {
        return &it->second;
    }
    return nullptr;
}

uint32_t TerritorialBehavior::isInTerritory(const glm::vec3& position, uint32_t excludeOwnerID) const {
    for (const auto& [ownerID, territory] : m_territories) {
        if (ownerID == excludeOwnerID) continue;

        float dist = glm::distance(position, territory.center);
        if (dist < territory.radius) {
            return ownerID;
        }
    }
    return 0;
}

float TerritorialBehavior::getAverageStrength() const {
    if (m_territories.empty()) return 0.0f;

    float totalStrength = 0.0f;
    for (const auto& [id, territory] : m_territories) {
        totalStrength += territory.strength;
    }
    return totalStrength / static_cast<float>(m_territories.size());
}

int TerritorialBehavior::getTotalIntrusions() const {
    int total = 0;
    for (const auto& [id, territory] : m_territories) {
        total += territory.intrusionCount;
    }
    return total;
}

void TerritorialBehavior::removeDeadOwnerTerritories(CreatureManager& creatures) {
    m_territoriesToRemove.clear();

    for (const auto& [ownerID, territory] : m_territories) {
        Creature* owner = creatures.getCreatureByID(ownerID);
        if (!owner || !owner->isAlive()) {
            m_territoriesToRemove.push_back(ownerID);
        }
    }

    for (uint32_t id : m_territoriesToRemove) {
        m_territories.erase(id);
    }
    m_territoriesToRemove.clear();
}

void TerritorialBehavior::updateTerritoryCenter(Territory& territory, const glm::vec3& ownerPos, float deltaTime) {
    // Territory center slowly drifts toward owner position
    glm::vec3 toOwner = ownerPos - territory.center;
    float dist = glm::length(toOwner);

    if (dist > 0.1f) {
        // Drift rate depends on how far owner is from center
        float driftRate = 0.1f;
        if (dist > territory.radius * 0.5f) {
            driftRate = 0.3f;  // Faster drift when owner is near edge
        }
        territory.center += toOwner * driftRate * deltaTime;
    }
}

void TerritorialBehavior::processIntrusions(Territory& territory, Creature* owner,
                                            const SpatialGrid& grid, float currentTime) {
    if (!owner) return;

    // Query creatures near territory center
    auto& nearby = grid.query(territory.center, territory.radius);

    for (Creature* other : nearby) {
        if (!other || other == owner || !other->isAlive()) continue;

        // Only same-type creatures are considered intruders for territorial disputes
        if (other->getType() == owner->getType()) {
            float dist = glm::distance(other->getPosition(), territory.center);
            if (dist < territory.radius * 0.8f) {  // Deep inside territory
                territory.intrusionCount++;
                territory.lastDefenseTime = currentTime;
            }
        }
    }

    // Cap intrusion count to prevent overflow
    territory.intrusionCount = glm::min(territory.intrusionCount, 20);
}

glm::vec3 TerritorialBehavior::calculateOwnerForce(Creature* owner, const Territory& territory,
                                                   const SpatialGrid& grid) {
    glm::vec3 force(0.0f);

    // Get nearby creatures
    auto& nearby = grid.query(territory.center, territory.radius);

    Creature* closestIntruder = nullptr;
    float closestDist = territory.radius;

    for (Creature* other : nearby) {
        if (!other || other == owner || !other->isAlive()) continue;

        // Same-type intruders trigger defense response
        if (other->getType() == owner->getType()) {
            float dist = glm::distance(other->getPosition(), territory.center);
            if (dist < closestDist) {
                closestDist = dist;
                closestIntruder = other;
            }
        }
    }

    if (closestIntruder) {
        // Chase the intruder!
        glm::vec3 toIntruder = closestIntruder->getPosition() - owner->getPosition();
        float dist = glm::length(toIntruder);

        if (dist > 0.5f) {
            // Aggression increases with intrusion count
            float aggression = 1.0f + static_cast<float>(territory.intrusionCount) /
                              static_cast<float>(m_config.maxIntrusionsBeforeAggression);
            aggression = glm::min(aggression, 2.0f);

            force = glm::normalize(toIntruder) * territory.strength *
                    m_config.defenseForceMultiplier * aggression;
        }
    } else {
        // No intruders - patrol behavior: tendency to stay near territory center
        glm::vec3 toCenter = territory.center - owner->getPosition();
        float distToCenter = glm::length(toCenter);

        if (distToCenter > territory.radius * 0.3f) {
            // Gently pull back toward center when too far
            force = glm::normalize(toCenter) * 0.3f * territory.strength;
        }
    }

    return force;
}

glm::vec3 TerritorialBehavior::calculateIntruderForce(Creature* intruder) {
    glm::vec3 force(0.0f);
    glm::vec3 pos = intruder->getPosition();

    for (const auto& [ownerID, territory] : m_territories) {
        glm::vec3 toCenter = territory.center - pos;
        float dist = glm::length(toCenter);

        if (dist < territory.radius && dist > 0.1f) {
            // Inside someone's territory - get pushed out
            // Force is stronger near center, weaker near edges
            float penetration = 1.0f - (dist / territory.radius);
            float repulsion = territory.strength * m_config.repulsionForceMultiplier * penetration;

            // Push away from center
            force -= glm::normalize(toCenter) * repulsion;
        }
    }

    return force;
}
