#include "ParentalCare.h"
#include "../Creature.h"
#include "../../core/CreatureManager.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>

bool ParentalCareBehavior::providesParentalCare(CreatureType type) {
    switch (type) {
        // Mammals and birds typically provide care
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::APEX_PREDATOR:
        case CreatureType::FLYING_BIRD:
        case CreatureType::AERIAL_PREDATOR:
        case CreatureType::AQUATIC_APEX:
            return true;

        // Some partial care
        case CreatureType::SMALL_PREDATOR:
        case CreatureType::OMNIVORE:
        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AMPHIBIAN:
            return true;

        // Little to no parental care
        case CreatureType::FRUGIVORE:
        case CreatureType::SCAVENGER:
        case CreatureType::FLYING_INSECT:
        case CreatureType::AQUATIC_HERBIVORE:
        case CreatureType::PARASITE:
        case CreatureType::CLEANER:
        default:
            return false;
    }
}

float ParentalCareBehavior::getCareDurationMultiplier(CreatureType type) {
    switch (type) {
        // Long-lived species with extended care
        case CreatureType::APEX_PREDATOR:
        case CreatureType::AQUATIC_APEX:
            return 1.5f;

        // Birds and aerial predators
        case CreatureType::FLYING_BIRD:
        case CreatureType::AERIAL_PREDATOR:
            return 1.2f;

        // Standard care
        case CreatureType::GRAZER:
        case CreatureType::BROWSER:
        case CreatureType::OMNIVORE:
            return 1.0f;

        // Shorter care periods
        case CreatureType::SMALL_PREDATOR:
        case CreatureType::AQUATIC_PREDATOR:
        case CreatureType::AMPHIBIAN:
            return 0.6f;

        // Minimal care
        default:
            return 0.3f;
    }
}

void ParentalCareBehavior::registerBirth(Creature* parent, Creature* child) {
    if (!parent || !child) return;
    if (!providesParentalCare(parent->getType())) return;

    ParentChild bond;
    bond.parentID = parent->getID();
    bond.childID = child->getID();
    bond.bondStartTime = m_currentTime;
    bond.bondStrength = 1.0f;
    bond.stage = CareStage::NURSING;
    bond.energyShared = 0.0f;
    bond.timeNearParent = 0.0f;
    bond.isDependent = true;

    size_t bondIndex = m_parentChildPairs.size();
    m_parentChildPairs.push_back(bond);

    m_childToParent[child->getID()] = bondIndex;
    m_parentToChildren[parent->getID()].push_back(bondIndex);

    // Register nest at parent's current location
    registerNest(parent->getID(), parent->getPosition());
}

void ParentalCareBehavior::update(float deltaTime, CreatureManager& creatures) {
    m_currentTime += deltaTime;

    // Clean up dead creatures first
    cleanupDeadCreatures(creatures);

    // Update each bond
    for (size_t i = 0; i < m_parentChildPairs.size(); i++) {
        ParentChild& bond = m_parentChildPairs[i];
        updateBond(bond, deltaTime, creatures);
    }

    // Remove expired bonds (reverse order)
    std::sort(m_bondsToRemove.begin(), m_bondsToRemove.end(), std::greater<size_t>());
    for (size_t idx : m_bondsToRemove) {
        if (idx < m_parentChildPairs.size()) {
            ParentChild& bond = m_parentChildPairs[idx];

            // Clean up mappings
            m_childToParent.erase(bond.childID);

            auto parentIt = m_parentToChildren.find(bond.parentID);
            if (parentIt != m_parentToChildren.end()) {
                auto& children = parentIt->second;
                children.erase(std::remove(children.begin(), children.end(), idx), children.end());
                if (children.empty()) {
                    m_parentToChildren.erase(parentIt);
                }
            }

            // Remove the bond
            m_parentChildPairs.erase(m_parentChildPairs.begin() + idx);

            // Update indices in maps (expensive, but bonds don't change frequently)
            for (auto& [childID, bondIdx] : m_childToParent) {
                if (bondIdx > idx) bondIdx--;
            }
            for (auto& [parentID, indices] : m_parentToChildren) {
                for (auto& i : indices) {
                    if (i > idx) i--;
                }
            }
        }
    }
    m_bondsToRemove.clear();

    // Update nest sites
    for (auto it = m_nestSites.begin(); it != m_nestSites.end(); ) {
        // Remove nests with no children or dead parent
        Creature* parent = creatures.getCreatureByID(it->parentID);
        if (!parent || !parent->isAlive() || it->childrenIDs.empty()) {
            it = m_nestSites.erase(it);
        } else {
            // Update children list
            it->childrenIDs.erase(
                std::remove_if(it->childrenIDs.begin(), it->childrenIDs.end(),
                              [&creatures](uint32_t id) {
                                  Creature* c = creatures.getCreatureByID(id);
                                  return !c || !c->isAlive();
                              }),
                it->childrenIDs.end());
            ++it;
        }
    }
}

glm::vec3 ParentalCareBehavior::calculateForce(Creature* creature, CreatureManager& creatures) {
    if (!creature || !creature->isAlive()) {
        return glm::vec3(0.0f);
    }

    uint32_t creatureID = creature->getID();
    glm::vec3 totalForce(0.0f);

    // Check if this is a parent
    auto parentIt = m_parentToChildren.find(creatureID);
    if (parentIt != m_parentToChildren.end() && !parentIt->second.empty()) {
        // Parent behavior
        for (size_t bondIdx : parentIt->second) {
            if (bondIdx >= m_parentChildPairs.size()) continue;

            const ParentChild& bond = m_parentChildPairs[bondIdx];
            Creature* child = creatures.getCreatureByID(bond.childID);

            if (child && child->isAlive() && bond.isDependent) {
                totalForce += calculateParentProtectionForce(bond, creature, child, creatures);
            }
        }
    }

    // Check if this is a dependent child
    auto childIt = m_childToParent.find(creatureID);
    if (childIt != m_childToParent.end()) {
        size_t bondIdx = childIt->second;
        if (bondIdx < m_parentChildPairs.size()) {
            const ParentChild& bond = m_parentChildPairs[bondIdx];
            if (bond.isDependent) {
                Creature* parent = creatures.getCreatureByID(bond.parentID);
                if (parent && parent->isAlive()) {
                    totalForce += calculateChildFollowForce(bond, creature, parent);
                }
            }
        }
    }

    return totalForce;
}

bool ParentalCareBehavior::isParent(uint32_t creatureID) const {
    auto it = m_parentToChildren.find(creatureID);
    return it != m_parentToChildren.end() && !it->second.empty();
}

bool ParentalCareBehavior::isDependent(uint32_t creatureID) const {
    auto it = m_childToParent.find(creatureID);
    if (it == m_childToParent.end()) return false;

    size_t bondIdx = it->second;
    if (bondIdx >= m_parentChildPairs.size()) return false;

    return m_parentChildPairs[bondIdx].isDependent;
}

uint32_t ParentalCareBehavior::getParentID(uint32_t childID) const {
    auto it = m_childToParent.find(childID);
    if (it == m_childToParent.end()) return 0;

    size_t bondIdx = it->second;
    if (bondIdx >= m_parentChildPairs.size()) return 0;

    return m_parentChildPairs[bondIdx].parentID;
}

std::vector<uint32_t> ParentalCareBehavior::getChildrenIDs(uint32_t parentID) const {
    std::vector<uint32_t> children;

    auto it = m_parentToChildren.find(parentID);
    if (it == m_parentToChildren.end()) return children;

    for (size_t bondIdx : it->second) {
        if (bondIdx < m_parentChildPairs.size()) {
            children.push_back(m_parentChildPairs[bondIdx].childID);
        }
    }

    return children;
}

const ParentalCareBehavior::ParentChild* ParentalCareBehavior::getBond(uint32_t childID) const {
    auto it = m_childToParent.find(childID);
    if (it == m_childToParent.end()) return nullptr;

    size_t bondIdx = it->second;
    if (bondIdx >= m_parentChildPairs.size()) return nullptr;

    return &m_parentChildPairs[bondIdx];
}

void ParentalCareBehavior::forceIndependence(uint32_t childID) {
    auto it = m_childToParent.find(childID);
    if (it == m_childToParent.end()) return;

    size_t bondIdx = it->second;
    if (bondIdx < m_parentChildPairs.size()) {
        m_parentChildPairs[bondIdx].isDependent = false;
        m_parentChildPairs[bondIdx].stage = CareStage::NONE;
        m_bondsToRemove.push_back(bondIdx);
    }
}

size_t ParentalCareBehavior::getActiveBondCount() const {
    size_t count = 0;
    for (const auto& bond : m_parentChildPairs) {
        if (bond.isDependent) count++;
    }
    return count;
}

float ParentalCareBehavior::getAverageBondStrength() const {
    if (m_parentChildPairs.empty()) return 0.0f;

    float total = 0.0f;
    for (const auto& bond : m_parentChildPairs) {
        total += bond.bondStrength;
    }
    return total / static_cast<float>(m_parentChildPairs.size());
}

void ParentalCareBehavior::updateBond(ParentChild& bond, float deltaTime,
                                      CreatureManager& creatures) {
    Creature* parent = creatures.getCreatureByID(bond.parentID);
    Creature* child = creatures.getCreatureByID(bond.childID);

    if (!parent || !parent->isAlive() || !child || !child->isAlive()) {
        // One of them died - break the bond
        bond.isDependent = false;
        return;
    }

    // Update time spent near parent
    float dist = glm::distance(parent->getPosition(), child->getPosition());
    float maxDist = m_config.careRadiusMultiplier * m_config.followDistance;

    if (dist < maxDist) {
        bond.timeNearParent += deltaTime;
    }

    // Decay bond strength over time
    bond.bondStrength -= m_config.bondDecayRate * deltaTime;
    bond.bondStrength = glm::max(0.0f, bond.bondStrength);

    // Update care stage
    updateCareStage(bond, parent, child);

    // Process energy sharing
    if (bond.stage == CareStage::NURSING || bond.stage == CareStage::GUARDING) {
        processEnergySharing(bond, parent, child, deltaTime);
    }

    // Check for independence
    if (shouldBecomeIndependent(bond, child)) {
        bond.isDependent = false;
        bond.stage = CareStage::NONE;
    }
}

void ParentalCareBehavior::updateCareStage(ParentChild& bond, Creature* parent, Creature* child) {
    float careTime = m_currentTime - bond.bondStartTime;
    float baseDuration = m_config.careDuration * getCareDurationMultiplier(parent->getType());

    // Progress through stages based on time
    float progress = careTime / baseDuration;

    if (progress < 0.2f) {
        bond.stage = CareStage::NESTING;
    } else if (progress < 0.5f) {
        bond.stage = CareStage::NURSING;
    } else if (progress < 0.75f) {
        bond.stage = CareStage::GUARDING;
    } else if (progress < 0.9f) {
        bond.stage = CareStage::TEACHING;
    } else {
        bond.stage = CareStage::WEANING;
    }

    // Override if bond is weak
    if (bond.bondStrength < m_config.weaningBondThreshold) {
        bond.stage = CareStage::WEANING;
    }
}

void ParentalCareBehavior::processEnergySharing(ParentChild& bond, Creature* parent,
                                                Creature* child, float deltaTime) {
    // Parent shares energy if they have enough and child needs it
    float parentEnergyRatio = parent->getEnergy() / 200.0f;  // Assume max 200
    float childEnergyRatio = child->getEnergy() / 200.0f;

    if (parentEnergyRatio > m_config.energyShareThreshold &&
        childEnergyRatio < m_config.childEnergyThreshold) {
        float shareAmount = m_config.energyShareRate * deltaTime * bond.bondStrength;

        // Cap at what parent can afford
        shareAmount = glm::min(shareAmount, parent->getEnergy() * 0.1f);

        // Transfer energy
        parent->consumeFood(-shareAmount);  // Negative = lose energy
        child->consumeFood(shareAmount);

        bond.energyShared += shareAmount;
        m_totalEnergyShared += shareAmount;
    }
}

glm::vec3 ParentalCareBehavior::calculateParentProtectionForce(const ParentChild& bond,
                                                               Creature* parent,
                                                               Creature* child,
                                                               CreatureManager& creatures) {
    glm::vec3 force(0.0f);
    glm::vec3 parentPos = parent->getPosition();
    glm::vec3 childPos = child->getPosition();

    // Stay near child
    glm::vec3 toChild = childPos - parentPos;
    float distToChild = glm::length(toChild);

    if (distToChild > m_config.followDistance * 2.0f && distToChild > 0.1f) {
        // Too far from child - move toward them
        force += glm::normalize(toChild) * 0.5f * bond.bondStrength;
    }

    // Look for threats near child
    auto& nearby = creatures.queryNearby(childPos, m_config.protectionRange);

    Creature* closestThreat = nullptr;
    float closestThreatDist = m_config.protectionRange;

    for (Creature* potential : nearby) {
        if (!potential || !potential->isAlive()) continue;
        if (potential->getID() == parent->getID()) continue;
        if (potential->getID() == child->getID()) continue;

        // Check if this is a predator threat
        if (isPredator(potential->getType())) {
            float dist = glm::distance(potential->getPosition(), childPos);
            if (dist < closestThreatDist) {
                closestThreatDist = dist;
                closestThreat = potential;
            }
        }
    }

    if (closestThreat) {
        // Move to intercept threat
        glm::vec3 threatPos = closestThreat->getPosition();
        glm::vec3 toThreat = threatPos - parentPos;
        float threatDist = glm::length(toThreat);

        if (threatDist > 0.1f && threatDist < m_config.protectionRange) {
            // Strong protective force
            force += glm::normalize(toThreat) * m_config.protectionForce * bond.bondStrength;
        }
    }

    return force;
}

glm::vec3 ParentalCareBehavior::calculateChildFollowForce(const ParentChild& bond,
                                                          Creature* child, Creature* parent) {
    glm::vec3 force(0.0f);
    glm::vec3 toParent = parent->getPosition() - child->getPosition();
    float dist = glm::length(toParent);

    if (dist < 0.1f) return force;

    float targetDist = m_config.followDistance;

    if (dist > targetDist) {
        // Too far - move toward parent
        float urgency = (dist - targetDist) / (m_config.careRadiusMultiplier * targetDist);
        urgency = glm::clamp(urgency, 0.0f, 1.0f);

        force = glm::normalize(toParent) * m_config.followForce * urgency * bond.bondStrength;
    } else if (dist < targetDist * 0.5f) {
        // Too close - give parent some space
        force = -glm::normalize(toParent) * 0.2f;
    }

    // Add some tendency to match parent velocity (follow along)
    glm::vec3 parentVel = parent->getVelocity();
    if (glm::length(parentVel) > 0.1f) {
        force += glm::normalize(parentVel) * 0.3f * bond.bondStrength;
    }

    return force;
}

bool ParentalCareBehavior::shouldBecomeIndependent(const ParentChild& bond, Creature* child) {
    // Age-based independence
    if (child->getAge() >= m_config.independenceAge) {
        return true;
    }

    // Bond too weak
    if (bond.bondStrength <= 0.05f) {
        return true;
    }

    // Stage is weaning and enough time has passed
    if (bond.stage == CareStage::WEANING) {
        float careTime = m_currentTime - bond.bondStartTime;
        if (careTime > m_config.careDuration * 0.95f) {
            return true;
        }
    }

    // Child has high energy and fitness (self-sufficient)
    if (child->getEnergy() > 150.0f && child->getFitness() > 0.5f) {
        if (bond.stage == CareStage::TEACHING || bond.stage == CareStage::WEANING) {
            return true;
        }
    }

    return false;
}

void ParentalCareBehavior::cleanupDeadCreatures(CreatureManager& creatures) {
    for (size_t i = 0; i < m_parentChildPairs.size(); i++) {
        ParentChild& bond = m_parentChildPairs[i];

        Creature* parent = creatures.getCreatureByID(bond.parentID);
        Creature* child = creatures.getCreatureByID(bond.childID);

        if (!parent || !parent->isAlive() || !child || !child->isAlive()) {
            m_bondsToRemove.push_back(i);
        }
    }
}

void ParentalCareBehavior::registerNest(uint32_t parentID, const glm::vec3& location) {
    // Check if parent already has a nest
    for (auto& nest : m_nestSites) {
        if (nest.parentID == parentID) {
            // Update existing nest
            return;
        }
    }

    // Create new nest
    NestSite nest;
    nest.parentID = parentID;
    nest.location = location;
    nest.established = m_currentTime;
    nest.safety = 1.0f;

    m_nestSites.push_back(nest);
}
